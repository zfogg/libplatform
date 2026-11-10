/**
 * @file platform/posix/process.c
 * @ingroup platform
 * @brief POSIX process execution utilities
 */

#ifndef _WIN32

/* Storage capacities in elements; text buffers include the terminating NUL. */
#define PLATFORM_POSIX_PROCESS_LINE_BUFFER_SIZE 256

#include <libplatform/process.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/resource.h>
#ifdef __APPLE__
#include <mach/mach.h>
#endif

int process_utf8_arguments(int *argc, char ***argv) {
  (void)argc;
  (void)argv;
  return 0;
}

int platform_process_stats(platform_process_stats_t *out) {
  if (!out)
    return -1;
  *out = (platform_process_stats_t){0};
  struct rusage usage;
  if (getrusage(RUSAGE_SELF, &usage))
    return -1;
  out->cpu_seconds = (double)usage.ru_utime.tv_sec + (double)usage.ru_stime.tv_sec +
                     ((double)usage.ru_utime.tv_usec + (double)usage.ru_stime.tv_usec) / 1e6;
#ifdef __APPLE__
  struct task_basic_info info;
  mach_msg_type_number_t count = TASK_BASIC_INFO_COUNT;
  if (task_info(mach_task_self(), TASK_BASIC_INFO, (task_info_t)&info, &count) == KERN_SUCCESS)
    out->resident_bytes = info.resident_size;
  thread_act_array_t threads;
  mach_msg_type_number_t n;
  if (task_threads(mach_task_self(), &threads, &n) == KERN_SUCCESS) {
    out->threads = n;
    for (mach_msg_type_number_t i = 0; i < n; ++i)
      mach_port_deallocate(mach_task_self(), threads[i]);
    vm_deallocate(mach_task_self(), (vm_address_t)threads, n * sizeof(*threads));
  }
#elif defined(__linux__)
  FILE *status = fopen("/proc/self/status", "r");
  if (status) {
    char line[PLATFORM_POSIX_PROCESS_LINE_BUFFER_SIZE];
    unsigned long long value;
    while (fgets(line, sizeof(line), status)) {
      if (sscanf(line, "VmRSS: %llu", &value) == 1)
        out->resident_bytes = value * 1024;
      if (sscanf(line, "Threads: %llu", &value) == 1)
        out->threads = value;
    }
    fclose(status);
  }
#endif
  return 0;
}

/**
 * @brief Get current process ID (POSIX implementation)
 * @return Process ID as pid_t
 */
pid_t platform_get_pid(void) {
  return getpid();
}

/**
 * @brief Execute a command and return a file stream (POSIX implementation)
 */
int platform_popen(const char *name, const char *command, const char *mode, FILE **out_stream) {
  if (!name || !command || !mode || !out_stream) {
    return EINVAL;
  }
  (void)name;
  FILE *stream = popen(command, mode);
  if (!stream) {
    return -1;
  }

  *out_stream = stream;
  return 0;
}

/**
 * @brief Close a process stream (POSIX implementation)
 */
int platform_pclose(FILE **stream_ptr) {
  if (!stream_ptr || !*stream_ptr) {
    return EINVAL;
  }

  int status = pclose(*stream_ptr);
  *stream_ptr = NULL;

  if (status == -1) {
    return -1;
  }

  return 0;
}

#endif
