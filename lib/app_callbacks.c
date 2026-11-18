#include <libplatform/app_callbacks.h>
#include <stddef.h>
#include <libplatform/atomic.h>

static platform_app_callbacks_t g_callbacks_storage;
static atomic_ptr_t g_callbacks;

void platform_app_callbacks_register(const platform_app_callbacks_t *callbacks) {
  if (callbacks == NULL) {
    atomic_ptr_store(&g_callbacks, NULL);
    return;
  }
  g_callbacks_storage = *callbacks;
  atomic_ptr_store(&g_callbacks, (void *)&g_callbacks_storage);
}

const platform_app_callbacks_t *platform_app_callbacks_get(void) {
  return (const platform_app_callbacks_t *)atomic_ptr_load(&g_callbacks);
}
