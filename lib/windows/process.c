/**
 * @file platform/windows/process.c
 * @ingroup platform
 * @brief Windows process execution utilities
 */

#ifdef _WIN32

#include <libplatform/process.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <windows.h>
#include <shellapi.h>
#include <psapi.h>
#include <tlhelp32.h>
#include <io.h>

struct platform_process {
  PROCESS_INFORMATION info;
};

static char *windows_command_line(const char *path, const char *const *argv) {
  size_t size = strlen(path) + 3;
  for (size_t i = 0; argv && argv[i]; ++i) size += strlen(argv[i]) + 4;
  char *line = malloc(size);
  if (!line) return NULL;
  size_t used = 0;
  used += (size_t)snprintf(line + used, size - used, "\"%s\"", path);
  for (size_t i = 1; argv && argv[i]; ++i)
    used += (size_t)snprintf(line + used, size - used, " \"%s\"", argv[i]);
  return line;
}

int platform_process_spawn(platform_process_t **out, const char *path, const char *const *argv,
                           int stdin_fd, int stdout_fd, int stderr_fd) {
  if (!out || !path || !argv) return -1;
  *out = NULL;
  char *command_line = windows_command_line(path, argv);
  if (!command_line) return -1;
  STARTUPINFOA startup = {.cb = sizeof(startup)};
  startup.dwFlags = STARTF_USESTDHANDLES;
  startup.hStdInput = stdin_fd >= 0 ? (HANDLE)_get_osfhandle(stdin_fd) : GetStdHandle(STD_INPUT_HANDLE);
  startup.hStdOutput = stdout_fd >= 0 ? (HANDLE)_get_osfhandle(stdout_fd) : GetStdHandle(STD_OUTPUT_HANDLE);
  startup.hStdError = stderr_fd >= 0 ? (HANDLE)_get_osfhandle(stderr_fd) : GetStdHandle(STD_ERROR_HANDLE);
  PROCESS_INFORMATION info = {0};
  BOOL ok = CreateProcessA(NULL, command_line, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &startup, &info);
  free(command_line);
  if (!ok) return -1;
  platform_process_t *process = calloc(1, sizeof(*process));
  if (!process) { CloseHandle(info.hThread); CloseHandle(info.hProcess); return -1; }
  process->info = info;
  *out = process;
  return 0;
}

int platform_process_wait(platform_process_t *process, int timeout_ms, int *exit_code_out) {
  if (!process) return -1;
  DWORD result = WaitForSingleObject(process->info.hProcess, timeout_ms < 0 ? INFINITE : (DWORD)timeout_ms);
  if (result == WAIT_TIMEOUT) return 1;
  if (result != WAIT_OBJECT_0) return -1;
  if (exit_code_out) { DWORD code = 0; GetExitCodeProcess(process->info.hProcess, &code); *exit_code_out = (int)code; }
  return 0;
}

bool platform_process_is_alive(platform_process_t *process) {
  if (!process) return false;
  return WaitForSingleObject(process->info.hProcess, 0) == WAIT_TIMEOUT;
}

int platform_process_kill(platform_process_t *process) {
  if (!process || !TerminateProcess(process->info.hProcess, 1)) return -1;
  return 0;
}

void platform_process_destroy(platform_process_t *process) {
  if (!process) return;
  CloseHandle(process->info.hThread);
  CloseHandle(process->info.hProcess);
  free(process);
}
// Windows passes a UTF-16 command line, while the narrow CRT argv uses the
// system code page. Keep UTF-8 arguments alive for the lifetime of the app.
static char windows_arg_data[131072];
static char *windows_args[32768];

int process_utf8_arguments(int *argc, char ***argv) {
  int count = 0;
  wchar_t **wide_args = CommandLineToArgvW(GetCommandLineW(), &count);
  if (!wide_args || count < 1 || count >= 32768) {
    if (wide_args)
      LocalFree(wide_args);
    return -1;
  }
  size_t used = 0;
  for (int i = 0; i < count; i++) {
    int length = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, wide_args[i], -1, NULL, 0, NULL, NULL);
    if (length <= 0 || (size_t)length > sizeof(windows_arg_data) - used) {
      LocalFree(wide_args);
      return -1;
    }
    windows_args[i] = windows_arg_data + used;
    if (!WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, wide_args[i], -1, windows_args[i], length, NULL, NULL)) {
      LocalFree(wide_args);
      return -1;
    }
    used += (size_t)length;
  }
  LocalFree(wide_args);
  windows_args[count] = NULL;
  *argc = count;
  *argv = windows_args;
  return 0;
}

int platform_process_stats(platform_process_stats_t *out) {
  if (!out)
    return -1;
  *out = (platform_process_stats_t){0};
  FILETIME created, exited, kernel, user;
  if (!GetProcessTimes(GetCurrentProcess(), &created, &exited, &kernel, &user))
    return -1;
  ULARGE_INTEGER k = {.LowPart = kernel.dwLowDateTime, .HighPart = kernel.dwHighDateTime};
  ULARGE_INTEGER u = {.LowPart = user.dwLowDateTime, .HighPart = user.dwHighDateTime};
  out->cpu_seconds = (double)(k.QuadPart + u.QuadPart) / 1e7;
  PROCESS_MEMORY_COUNTERS memory = {.cb = sizeof(memory)};
  if (K32GetProcessMemoryInfo(GetCurrentProcess(), &memory, sizeof(memory)))
    out->resident_bytes = memory.WorkingSetSize;
  HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
  if (snapshot != INVALID_HANDLE_VALUE) {
    THREADENTRY32 entry = {.dwSize = sizeof(entry)};
    if (Thread32First(snapshot, &entry))
      do {
        if (entry.th32OwnerProcessID == GetCurrentProcessId())
          ++out->threads;
      } while (Thread32Next(snapshot, &entry));
    CloseHandle(snapshot);
  }
  return 0;
}

int platform_process_list(const char *name_substring, platform_process_list_callback_t callback, void *context) {
  if (!callback) return -1;
  HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
  if (snapshot == INVALID_HANDLE_VALUE) return -1;
  PROCESSENTRY32 entry = {.dwSize = sizeof(entry)};
  int result = 0;
  if (Process32First(snapshot, &entry)) {
    do {
      if (platform_process_name_matches(entry.szExeFile, name_substring)) {
        platform_process_info_t info = {.pid = (pid_t)entry.th32ProcessID};
        strncpy(info.name, entry.szExeFile, sizeof(info.name) - 1);
        if (callback(&info, context)) break;
      }
    } while (Process32Next(snapshot, &entry));
  } else {
    result = -1;
  }
  CloseHandle(snapshot);
  return result;
}

/**
 * @brief Get current process ID (Windows implementation)
 * @return Process ID as pid_t
 */
pid_t platform_get_pid(void) {
  return (pid_t)GetCurrentProcessId();
}

/**
 * @brief Execute a command and return a file stream (Windows implementation)
 */
int platform_popen(const char *name, const char *command, const char *mode, FILE **out_stream) {
  if (!name || !command || !mode || !out_stream) {
    return -1;
  }

  FILE *stream = NULL;
  (void)name;
  stream = _popen(command, mode);
  if (!stream) {
    return -1;
  }

  int fd = _fileno(stream);
  if (fd >= 0) {
    (void)fd;
  }

  *out_stream = stream;
  return 0;
}

/**
 * @brief Close a process stream (Windows implementation)
 */
int platform_pclose(FILE **stream_ptr) {
  if (!stream_ptr || !*stream_ptr) {
    return -1;
  }

  int status = _pclose(*stream_ptr);
  *stream_ptr = NULL;

  if (status == -1) {
    return -1;
  }

  return 0;
}

#endif
