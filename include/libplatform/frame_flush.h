#pragma once

#include <libplatform/compiler.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*platform_frame_flush_callback_t)(void);

void platform_frame_flush_set_callback(platform_frame_flush_callback_t callback);
int platform_frame_flush(int fd);

#ifdef __cplusplus
}
#endif
