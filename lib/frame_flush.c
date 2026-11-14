#include <libplatform/frame_flush.h>

int platform_frame_flush_native(int fd);

static platform_frame_flush_callback_t g_frame_flush_callback;

void platform_frame_flush_set_callback(platform_frame_flush_callback_t callback) {
  g_frame_flush_callback = callback;
}

int platform_frame_flush(int fd) {
  if (g_frame_flush_callback) {
    g_frame_flush_callback();
    return 0;
  }
  return platform_frame_flush_native(fd);
}
