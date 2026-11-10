#pragma once
#include <stdbool.h>
#include <stdint.h>
typedef struct platform_wakeup platform_wakeup_t;
int platform_wakeup_create(platform_wakeup_t **out);
void platform_wakeup_signal(platform_wakeup_t *wake);
bool platform_wakeup_wait(platform_wakeup_t *wake, uint64_t timeout_ns);
void platform_wakeup_destroy(platform_wakeup_t *wake);
