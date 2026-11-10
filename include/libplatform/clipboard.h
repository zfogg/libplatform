#pragma once
#include <stddef.h>

typedef int (*platform_clipboard_format_paths_fn)(const char *const *paths, size_t count, char *output,
                                                   size_t capacity);
void platform_clipboard_set_path_formatter(platform_clipboard_format_paths_fn formatter);
int platform_clipboard_read(char *output, size_t capacity);
