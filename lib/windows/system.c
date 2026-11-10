#ifdef _WIN32

#include <libplatform/system.h>
#include <libplatform/filesystem.h>
#include <libplatform/terminal.h>
#include <windows.h>
#include <psapi.h>
#include <fcntl.h>
#include <io.h>
#include <stdarg.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <errno.h>
#include <sys/stat.h>
#include <direct.h>
#include <process.h>

const char *platform_getenv(const char *name) {
  return name && name[0] ? getenv(name) : NULL;
}

int platform_open(const char *name, const char *pathname, int flags, ...) {
  (void)name;
  if (!pathname)
    return -1;
  int mode = 0;
  if (flags & _O_CREAT) {
    va_list args;
    va_start(args, flags);
    mode = va_arg(args, int);
    va_end(args);
  }
  if (!(flags & _O_TEXT))
    flags |= _O_BINARY;
  return _open(pathname, flags, mode);
}

int get_binary_file_address_offsets(const void *addr, platform_binary_match_t *matches, int max_matches) {
  HANDLE proc = GetCurrentProcess();
  HMODULE modules[512];
  DWORD needed = 0;
  int count = 0;
  uintptr_t addr_int = (uintptr_t)addr;

  if (!EnumProcessModules(proc, modules, sizeof(modules), &needed)) {
    return 0;
  }

  DWORD module_count = needed / sizeof(HMODULE);
  if (module_count > 512) {
    module_count = 512; // Cap at our array size
  }

  for (DWORD i = 0; i < module_count && count < max_matches; i++) {
    MODULEINFO mod_info;
    char mod_path[PLATFORM_MAX_PATH_LENGTH];

    if (!GetModuleInformation(proc, modules[i], &mod_info, sizeof(mod_info))) {
      continue;
    }

    if (!GetModuleFileNameA(modules[i], mod_path, sizeof(mod_path))) {
      continue;
    }

    // Check if address falls within this module
    uintptr_t base = (uintptr_t)mod_info.lpBaseOfDll;
    uintptr_t end = base + mod_info.SizeOfImage;

    if (addr_int >= base && addr_int < end) {
      strncpy_s(matches[count].path, PLATFORM_MAX_PATH_LENGTH, mod_path, PLATFORM_MAX_PATH_LENGTH - 1);
      matches[count].path[PLATFORM_MAX_PATH_LENGTH - 1] = '\0';
      matches[count].symbolizer_address = addr_int - base;
      matches[count].relative_address = true;

      count++;
    }
  }

  return count;
}

int platform_get_last_error(void) { return (int)GetLastError(); }
int platform_init(void) { return 0; }
void platform_destroy(void) { platform_cleanup_binary_path_cache(); }
int platform_restore_timer_resolution(void) { return 0; }
static console_ctrl_handler_t g_console_ctrl_handler = NULL;
static BOOL WINAPI platform_console_ctrl_handler(DWORD control_type) {
  if (!g_console_ctrl_handler)
    return FALSE;
  switch (control_type) {
  case CTRL_C_EVENT:
    return g_console_ctrl_handler(CONSOLE_CTRL_C) ? TRUE : FALSE;
  case CTRL_BREAK_EVENT:
    return g_console_ctrl_handler(CONSOLE_CTRL_BREAK) ? TRUE : FALSE;
  case CTRL_CLOSE_EVENT:
  case CTRL_LOGOFF_EVENT:
  case CTRL_SHUTDOWN_EVENT:
    return g_console_ctrl_handler(CONSOLE_CLOSE) ? TRUE : FALSE;
  default:
    return FALSE;
  }
}
bool platform_set_console_ctrl_handler(console_ctrl_handler_t handler) {
  if (handler) {
    g_console_ctrl_handler = handler;
    if (!SetConsoleCtrlHandler(platform_console_ctrl_handler, TRUE)) {
      g_console_ctrl_handler = NULL;
      return false;
    }
    return true;
  }
  g_console_ctrl_handler = NULL;
  return SetConsoleCtrlHandler(platform_console_ctrl_handler, FALSE) != 0;
}
uint64_t platform_get_monotonic_time_us(void) {
  LARGE_INTEGER now, frequency;
  QueryPerformanceCounter(&now); QueryPerformanceFrequency(&frequency);
  return (uint64_t)((now.QuadPart * 1000000ULL) / frequency.QuadPart);
}
void platform_sleep_ms(unsigned int ms) { Sleep(ms); }
void platform_sleep_us(unsigned int us) { Sleep((us + 999U) / 1000U); }
void platform_sleep_ns(uint64_t ns) { Sleep((DWORD)((ns + 999999ULL) / 1000000ULL)); }
int platform_memcpy(void *dest, size_t dest_size, const void *src, size_t count) {
  return (!dest || !src || count > dest_size) ? -1 : (memcpy(dest, src, count), 0);
}
int platform_memset(void *dest, size_t dest_size, int value, size_t count) {
  if (!dest || count > dest_size) return -1; memset(dest, value, count); return 0;
}
int platform_access(const char *path, int mode) { return path ? _access(path, mode) : -1; }
ssize_t platform_read(int fd, void *buf, size_t count) { return _read(fd, buf, (unsigned int)count); }
ssize_t platform_write(int fd, const void *buf, size_t count) { return _write(fd, buf, (unsigned int)count); }
FILE *platform_tmpfile(void) { return tmpfile(); }
void platform_force_exit(int exit_code) { ExitProcess((UINT)exit_code); }
void platform_raise_fd_limit(unsigned int limit) { (void)limit; }
signal_handler_t platform_signal(int sig, signal_handler_t handler) { return signal(sig, handler); }
platform_stderr_redirect_handle_t platform_stderr_redirect_to_null(void) { return (platform_stderr_redirect_handle_t){-1, -1}; }
void platform_stderr_restore(platform_stderr_redirect_handle_t handle) { (void)handle; }
platform_stderr_redirect_handle_t platform_stdout_stderr_redirect_to_null(void) { return (platform_stderr_redirect_handle_t){-1, -1}; }
void platform_stdout_stderr_restore(platform_stderr_redirect_handle_t handle) { (void)handle; }
FILE *platform_fopen(const char *name, const char *filename, const char *mode) { (void)name; return filename && mode ? fopen(filename, mode) : NULL; }
FILE *platform_fdopen(const char *name, int fd, const char *mode) { (void)name; return mode ? _fdopen(fd, mode) : NULL; }
int platform_close(int fd) { return _close(fd); }
int platform_unlink(const char *pathname) { return pathname ? _unlink(pathname) : -1; }
int platform_chmod(const char *pathname, int mode) { return pathname ? _chmod(pathname, mode) : -1; }
int platform_localtime(const time_t *timer, struct tm *result) { return timer && result ? localtime_s(result, timer) : EINVAL; }
int platform_gtime(const time_t *timer, struct tm *result) { return timer && result ? gmtime_s(result, timer) : EINVAL; }
int platform_setenv(const char *name, const char *value) { return name && value ? _putenv_s(name, value) : -1; }
int platform_unsetenv(const char *name) { return name ? _putenv_s(name, "") : -1; }
const char *platform_strerror(int errnum) { return strerror(errnum); }
bool platform_get_cwd(char *cwd, size_t path_size) { return cwd && path_size && _getcwd(cwd, (int)path_size) != NULL; }
bool platform_get_temp_dir(char *temp_dir, size_t path_size) { return temp_dir && path_size && GetTempPathA((DWORD)path_size, temp_dir) != 0; }
int platform_execute_subprocess(const char *executable, const char **argv, char *output_buffer, size_t output_size,
                                bool (*cancel)(void *), void *data) {
  (void)executable; (void)argv; (void)output_buffer; (void)output_size; (void)cancel; (void)data; return -1;
}

int platform_load_system_ca_certs(char **pem_data_out, size_t *pem_size_out) {
  if (pem_data_out) *pem_data_out = NULL;
  if (pem_size_out) *pem_size_out = 0;
  return -1;
}

int platform_resolve_hostname_to_ipv4(const char *hostname, char *ipv4_out, size_t ipv4_out_size) {
  (void)hostname;
  if (ipv4_out && ipv4_out_size) ipv4_out[0] = '\0';
  return -1;
}

#endif
