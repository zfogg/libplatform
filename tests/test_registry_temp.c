#include <libplatform/filesystem.h>
#include <libplatform/named.h>
#include <stdint.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#endif

int main(void) {
  int missing = 0, present = 0;
  if (named_init() != 0)
    return 1;
  named_unregister((uintptr_t)&missing);
  if (!named_register_fmt((uintptr_t)&present, "fixture", "%p", __FILE__,
                          __LINE__, __func__, "present") ||
      !named_get((uintptr_t)&present))
    return 2;
  named_unregister((uintptr_t)&present);
#ifdef _WIN32
  char expected[MAX_PATH + 1], actual[32767], tiny[1];
  DWORD length = GetTempPathA(sizeof(expected), expected);
  if (!length || length >= sizeof(expected))
    return 3;
  if (!platform_get_temp_dir(actual, sizeof(actual)) ||
      strcmp(actual, expected))
    return 4;
  if (platform_get_temp_dir(tiny, sizeof(tiny)) ||
      platform_get_temp_dir(NULL, 1) || platform_get_temp_dir(actual, 0))
    return 5;
#endif
  return 0;
}
