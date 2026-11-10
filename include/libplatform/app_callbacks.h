#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*libplatform_debug_write_fn)(const char *text, void *context);
typedef struct {
  libplatform_debug_write_fn write_sync_state;
  libplatform_debug_write_fn write_memory_report;
  libplatform_debug_write_fn write_backtrace;
  void *debug_context;
} libplatform_app_callbacks_t;

void libplatform_app_callbacks_register(const libplatform_app_callbacks_t *callbacks);
const libplatform_app_callbacks_t *libplatform_app_callbacks_get(void);

#ifdef __cplusplus
}
#endif
