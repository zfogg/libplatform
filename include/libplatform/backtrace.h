#pragma once

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define LIBPLATFORM_BACKTRACE_MAX_FRAMES 64

/* Captures the current thread's stack. Returns the number of frames written. */
int libplatform_backtrace_capture(void **frames, int capacity);

/* Returns one heap-allocated, human-readable string per frame plus a NULL terminator. */
char **libplatform_backtrace_symbolize(void *const *frames, int count);
void libplatform_backtrace_symbols_free(char **symbols);

/* Optional application-owned output hook; the library never logs or writes output itself. */
typedef void (*libplatform_backtrace_write_fn)(const char *line, void *context);
void libplatform_backtrace_print(int skip_frames, libplatform_backtrace_write_fn write, void *context);

#ifdef __cplusplus
}
#endif
