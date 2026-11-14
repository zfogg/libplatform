#include <libplatform/frame_flush.h>
#include <libplatform/terminal.h>

int platform_frame_flush_native(int fd) {
  return terminal_flush(fd);
}
