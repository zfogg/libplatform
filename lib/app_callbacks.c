#include <libplatform/app_callbacks.h>
#include <stddef.h>
#include <stdatomic.h>

static platform_app_callbacks_t g_callbacks_storage;
static _Atomic(const platform_app_callbacks_t *) g_callbacks;

void platform_app_callbacks_register(const platform_app_callbacks_t *callbacks) {
  if (callbacks == NULL) {
    atomic_store_explicit(&g_callbacks, NULL, memory_order_release);
    return;
  }
  g_callbacks_storage = *callbacks;
  atomic_store_explicit(&g_callbacks, &g_callbacks_storage, memory_order_release);
}

const platform_app_callbacks_t *platform_app_callbacks_get(void) {
  return atomic_load_explicit(&g_callbacks, memory_order_acquire);
}
