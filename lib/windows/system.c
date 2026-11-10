#ifdef _WIN32

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
