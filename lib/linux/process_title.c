/** @file platform/linux/process_title.c Linux process comm naming. */
#include "../process_title_internal.h"
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <unistd.h>

int platform_process_title_native_set(const char *title, const char *short_name) {
  (void)title;
  if (syscall(SYS_gettid) != getpid()) {
    return -1;
  }
  // PR_SET_NAME truncates to 15 bytes and changes only the calling thread.
  // On the main thread this also updates /proc/PID/comm.
  if (prctl(PR_SET_NAME, short_name, 0UL, 0UL, 0UL) != 0) {
    return -1;
  }
  return 0;
}
