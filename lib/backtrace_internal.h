#pragma once

#include <libplatform/backtrace.h>

int platform_backtrace_capture(void **buffer, int size);
char **platform_backtrace_symbols(void *const *buffer, int size, const char *const *owned_dirs);
void platform_backtrace_symbols_destroy(char **strings);
void platform_backtrace_print_simple(int skip_frames, platform_backtrace_write_fn write,
                                     void *context);
