#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*platform_debug_write_fn)(const char *text, void *context);
typedef struct {
  platform_debug_write_fn write_sync_state;
  platform_debug_write_fn write_memory_report;
  platform_debug_write_fn write_backtrace;
  void *debug_context;
} platform_app_callbacks_t;

void platform_app_callbacks_register(const platform_app_callbacks_t *callbacks);
const platform_app_callbacks_t *platform_app_callbacks_get(void);

#ifdef __cplusplus
}
#endif
