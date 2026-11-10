#ifdef _WIN32

#include <stdint.h>
#include <windows.h>
#include <errno.h>
#include <io.h>
#include <direct.h>
#include <string.h>
#include <time.h>
#include <libplatform/filesystem.h>
#include <libplatform/system.h>

int platform_get_last_error(void) { return (int)GetLastError(); }

bool platform_get_temp_dir(char *temp_dir, size_t path_size) {
  if (!temp_dir || path_size == 0) return false;
  DWORD length = GetTempPathA((DWORD)path_size, temp_dir);
  return length != 0 && length < path_size;
}

bool platform_get_cwd(char *cwd, size_t path_size) {
  if (!cwd || path_size == 0) return false;
  return GetCurrentDirectoryA((DWORD)path_size, cwd) != 0;
}

int platform_access(const char *path, int mode) {
  int windows_mode = 0;
  if (mode & PLATFORM_ACCESS_READ) windows_mode |= 4;
  if (mode & PLATFORM_ACCESS_WRITE) windows_mode |= 2;
  return _access(path, windows_mode);
}

uint64_t platform_get_monotonic_time_us(void) {
  static LARGE_INTEGER frequency;
  static INIT_ONCE once = INIT_ONCE_STATIC_INIT;
  BOOL pending = FALSE;
  InitOnceBeginInitialize(&once, 0, &pending, NULL);
  if (pending) {
    QueryPerformanceFrequency(&frequency);
    InitOnceComplete(&once, 0, NULL);
  }
  LARGE_INTEGER counter;
  QueryPerformanceCounter(&counter);
  return (uint64_t)((counter.QuadPart * 1000000ULL) / frequency.QuadPart);
}

int platform_localtime(const time_t *timer, struct tm *result) {
  return localtime_s(result, timer) == 0 ? 0 : -1;
}

int platform_setenv(const char *name, const char *value) {
  return _putenv_s(name, value ? value : "");
}

int platform_unsetenv(const char *name) { return _putenv_s(name, ""); }

int platform_memcpy(void *dest, size_t dest_size, const void *src, size_t count) {
  if (!dest || !src || count > dest_size) return -1;
  return memcpy_s(dest, dest_size, src, count) == 0 ? 0 : -1;
}

int platform_memset(void *dest, size_t dest_size, int ch, size_t count) {
  if (!dest || count > dest_size) return -1;
  return memset_s(dest, dest_size, ch, count) == 0 ? 0 : -1;
}

FILE *platform_fopen(const char *name, const char *filename, const char *mode) {
  (void)name;
  FILE *file = NULL;
  return fopen_s(&file, filename, mode) == 0 ? file : NULL;
}

void platform_sleep_us(unsigned int us) {
  DWORD timeout_ms = (DWORD)((us + 999U) / 1000U);
  if (timeout_ms < 1)
    timeout_ms = 1;
  Sleep(timeout_ms);
}

void platform_sleep_ns(uint64_t ns) {
  if (ns == 0)
    return;
  HANDLE timer = CreateWaitableTimerW(NULL, TRUE, NULL);
  if (!timer) {
    platform_sleep_us((unsigned int)((ns + 999U) / 1000U));
    return;
  }
  uint64_t units = (ns + 99U) / 100U;
  LARGE_INTEGER due_time;
  due_time.QuadPart = -(units > (uint64_t)INT64_MAX ? INT64_MAX : (int64_t)units);
  if (SetWaitableTimer(timer, &due_time, 0, NULL, NULL, FALSE))
    WaitForSingleObject(timer, INFINITE);
  CloseHandle(timer);
}

#include <libplatform/system.h>
#include <windows.h>
#include <psapi.h>
#include <fcntl.h>
#include <io.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

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

#endif
