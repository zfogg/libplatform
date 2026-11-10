#ifdef _WIN32
#include <libplatform/wakeup.h>
#include <windows.h>
#include <stdatomic.h>
#include <stdlib.h>
struct platform_wakeup { HANDLE event; atomic_bool pending; };
int platform_wakeup_create(platform_wakeup_t **out) {
  if (!out) return -1;
  platform_wakeup_t *wake = malloc(sizeof(*wake));
  if (!wake) return -1;
  wake->event = CreateEventW(NULL, FALSE, FALSE, NULL);
  if (!wake->event) { free(wake); return -1; }
  atomic_init(&wake->pending, false); *out = wake; return 0;
}
void platform_wakeup_signal(platform_wakeup_t *wake) {
  if (wake && !atomic_exchange(&wake->pending, true)) SetEvent(wake->event);
}
bool platform_wakeup_wait(platform_wakeup_t *wake, uint64_t timeout_ns) {
  if (!wake) return false;
  DWORD timeout = timeout_ns > 0xffffffffULL * 1000000ULL ? INFINITE : (DWORD)((timeout_ns + 999999ULL) / 1000000ULL);
  if (WaitForSingleObject(wake->event, timeout) != WAIT_OBJECT_0) return false;
  atomic_store(&wake->pending, false); return true;
}
void platform_wakeup_destroy(platform_wakeup_t *wake) { if (wake) { CloseHandle(wake->event); free(wake); } }
#endif
