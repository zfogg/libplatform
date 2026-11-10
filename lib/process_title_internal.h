#pragma once
#include <stddef.h>

// Main-thread-only OS naming surfaces beyond the argv command line.
int platform_process_title_native_set(const char *title, const char *short_name);
