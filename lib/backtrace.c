/**
 * @file platform/posix/backtrace.c
 * @brief POSIX backtrace implementation using libexecinfo and addr2line
 * @ingroup platform
 */

#include <libplatform/backtrace.h>
#include <libplatform/symbols.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#if (defined(__APPLE__) || defined(__linux__) || defined(__unix__)) && \
    (!defined(__has_include) || __has_include(<execinfo.h>))
#include <execinfo.h>
#define LIBPLATFORM_HAS_EXECINFO 1
#endif

/**
 * @brief Manual stack unwinding using frame pointers
 *
 * Falls back to manual frame pointer walking if libexecinfo's backtrace()
 * is unavailable or fails (e.g., on musl libc without libexecinfo).
 */
static int manual_backtrace(void **buffer, int size) {
  if (!buffer || size <= 0) {
    return 0;
  }

  void **frame = (void **)__builtin_frame_address(0);
  int depth = 0;

  while (frame && depth < size) {
    void *return_addr = frame[1];

    if (!return_addr || return_addr < (void *)0x1000) {
      break;
    }

    buffer[depth++] = return_addr;

    void **prev_frame = (void **)frame[0];

    if (!prev_frame || prev_frame <= frame || (uintptr_t)prev_frame & 0x7) {
      break;
    }

    frame = prev_frame;
  }

  return depth;
}

/**
 * @brief Safe wrapper for backtrace() with weak symbol check
 *
 * On musl libc, backtrace is a weak symbol that may not be available.
 * This wrapper checks for NULL before calling.
 */
static inline int safe_backtrace(void **buffer, int size) {
#if defined(LIBPLATFORM_HAS_EXECINFO)
  if (backtrace != NULL) {
    return backtrace(buffer, size);
  }
  return 0;
#else
  (void)buffer;
  (void)size;
  return 0;
#endif
}

#ifndef _WIN32
int platform_backtrace_capture(void **buffer, int size) {
  int depth = safe_backtrace(buffer, size);

  if (depth == 0) {
    depth = manual_backtrace(buffer, size);
  }

  return depth;
}

char **platform_backtrace_symbols(void *const *buffer, int size) {
  return symbol_cache_resolve_batch(buffer, size);
}

void platform_backtrace_symbols_destroy(char **strings) {
  if (!strings) return;
  symbol_cache_free_symbols(strings);
}
#endif

#ifndef _WIN32
void platform_backtrace_print(int skip_frames, platform_backtrace_write_fn write, void *context) {
  void *buffer[LIBPLATFORM_BACKTRACE_MAX_FRAMES];
  int depth = platform_backtrace_capture(buffer, LIBPLATFORM_BACKTRACE_MAX_FRAMES);

  if (depth > 0) {
    char **symbols = platform_backtrace_symbols((void *const *)buffer, depth);
    if (symbols) {
      for (int i = skip_frames; i < depth && symbols[i]; i++) {
        if (write) {
          char line[1024];
          (void)snprintf(line, sizeof(line), "  [%d] %s", i - skip_frames, symbols[i]);
          write(line, context);
        }
      }
      platform_backtrace_symbols_destroy(symbols);
    }
  }
}
#endif
