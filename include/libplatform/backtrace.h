#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define LIBPLATFORM_BACKTRACE_MAX_FRAMES 64

/* Captures the current thread's stack. Returns the number of frames written. */
int platform_backtrace_capture(void **frames, int capacity);

/* Returns one heap-allocated, human-readable string per frame plus a NULL terminator. */
char **platform_backtrace_symbols(void *const *frames, int count, const char *const *owned_dirs);
void platform_backtrace_symbols_destroy(char **symbols);

/* Optional application-owned output hook; the library never logs or writes output itself. */
typedef void (*platform_backtrace_write_fn)(const char *line, void *context);
void platform_backtrace_print(int skip_frames, platform_backtrace_write_fn write, void *context);

#ifdef __cplusplus
}
#endif
