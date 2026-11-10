/**
 * @file platform/windows/keepawake.c
 * @brief Windows system sleep prevention using SetThreadExecutionState
 * @ingroup platform
 */

#include <libplatform/system.h>
#include <windows.h>

int platform_enable_keepawake(void) {
  // Windows: Use SetThreadExecutionState
  // Note: SetThreadExecutionState returns the PREVIOUS state, not an error code.
  // It cannot fail - it always succeeds. The return value is informational only.
  SetThreadExecutionState(ES_CONTINUOUS | ES_SYSTEM_REQUIRED | ES_DISPLAY_REQUIRED);

  return 0;
}

void platform_disable_keepawake(void) {
  // Clear all execution state flags (allow sleep)
  SetThreadExecutionState(ES_CONTINUOUS);
}
