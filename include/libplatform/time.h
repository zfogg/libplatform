#pragma once
#include <stddef.h>
#include <stdint.h>
#include <time.h>
uint64_t time_get_ns(void);
uint64_t time_get_realtime_ns(void);
void time_ns_to_timespec(uint64_t nanoseconds, struct timespec *result);
void time_pretty(uint64_t nanoseconds, int precision, char *output, size_t capacity);
