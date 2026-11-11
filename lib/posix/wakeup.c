#ifndef _WIN32

#include <libplatform/wakeup.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <poll.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <unistd.h>

struct platform_wakeup { int read_fd; int write_fd; atomic_bool pending; };

int platform_wakeup_create(platform_wakeup_t **out) {
  if (!out) return -1;
  *out = NULL;
  int fds[2];
  if (pipe(fds) != 0) return -1;
  (void)fcntl(fds[0], F_SETFL, fcntl(fds[0], F_GETFL, 0) | O_NONBLOCK);
  (void)fcntl(fds[1], F_SETFL, fcntl(fds[1], F_GETFL, 0) | O_NONBLOCK);
  platform_wakeup_t *wake = calloc(1, sizeof(*wake));
  if (!wake) { close(fds[0]); close(fds[1]); return -1; }
  wake->read_fd = fds[0];
  wake->write_fd = fds[1];
  atomic_init(&wake->pending, false);
  *out = wake;
  return 0;
}

void platform_wakeup_signal(platform_wakeup_t *wake) {
  if (!wake || atomic_exchange(&wake->pending, true)) return;
  const unsigned char signal = 1;
  if (write(wake->write_fd, &signal, sizeof(signal)) < 0 && errno != EAGAIN)
    atomic_store(&wake->pending, false);
}

bool platform_wakeup_wait(platform_wakeup_t *wake, uint64_t timeout_ns) {
  if (!wake) return false;
  uint64_t timeout_ms = (timeout_ns + 999999ULL) / 1000000ULL;
  int timeout = timeout_ms > (uint64_t)INT_MAX ? INT_MAX : (int)timeout_ms;
  struct pollfd descriptor = {.fd = wake->read_fd, .events = POLLIN};
  if (poll(&descriptor, 1, timeout) <= 0) return false;
  unsigned char buffer[64];
  while (read(wake->read_fd, buffer, sizeof(buffer)) > 0) {}
  atomic_store(&wake->pending, false);
  return true;
}

void platform_wakeup_destroy(platform_wakeup_t *wake) {
  if (!wake) return;
  close(wake->read_fd); close(wake->write_fd); free(wake);
}

#endif
