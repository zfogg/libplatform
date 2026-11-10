#include <libplatform/backtrace.h>
#include <libplatform/symbols.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#include <windows.h>
#elif defined(__EMSCRIPTEN__)
#elif (defined(__APPLE__) || defined(__linux__) || defined(__unix__)) && \
    (!defined(__has_include) || __has_include(<execinfo.h>) )
#include <dlfcn.h>
#include <execinfo.h>
#define LIBPLATFORM_HAS_EXECINFO 1
#endif

static char *dup_string(const char *s) {
  size_t n = strlen(s) + 1;
  char *copy = (char *)malloc(n);
  if (copy) memcpy(copy, s, n);
  return copy;
}

int libplatform_backtrace_capture(void **frames, int capacity) {
  if (!frames || capacity <= 0) return 0;
#if defined(_WIN32)
  return (int)CaptureStackBackTrace(1, (DWORD)capacity, frames, NULL);
#elif defined(__EMSCRIPTEN__)
  (void)frames; (void)capacity; return 0;
#elif defined(LIBPLATFORM_HAS_EXECINFO)
  return backtrace(frames, capacity);
#else
  return 0;
#endif
}

char **libplatform_backtrace_symbolize(void *const *frames, int count) {
  if (!frames || count <= 0) return NULL;
#if !defined(__EMSCRIPTEN__)
  if (symbol_cache_init() == LIBPLATFORM_OK) return symbol_cache_resolve_batch(frames, count);
#endif
  char **out = (char **)calloc((size_t)count + 1, sizeof(*out));
  if (!out) return NULL;
#if defined(__EMSCRIPTEN__)
  for (int i = 0; i < count; ++i) { char buf[64]; snprintf(buf, sizeof(buf), "%p", frames[i]); out[i] = dup_string(buf); }
#elif defined(_WIN32)
  for (int i = 0; i < count; ++i) { char buf[64]; snprintf(buf, sizeof(buf), "%p", frames[i]); out[i] = dup_string(buf); }
#elif defined(LIBPLATFORM_HAS_EXECINFO)
  char **native = backtrace_symbols(frames, count);
  if (native) { for (int i = 0; i < count; ++i) out[i] = dup_string(native[i]); free(native); }
  else { for (int i = 0; i < count; ++i) { char buf[64]; snprintf(buf, sizeof(buf), "%p", frames[i]); out[i] = dup_string(buf); } }
#endif
  return out;
}

void libplatform_backtrace_symbols_free(char **symbols) {
  if (!symbols) return;
#if !defined(__EMSCRIPTEN__)
  symbol_cache_free_symbols(symbols);
  return;
#endif
  for (size_t i = 0; symbols[i]; ++i) free(symbols[i]);
  free(symbols);
}

void libplatform_backtrace_print(int skip_frames, libplatform_backtrace_write_fn write, void *context) {
  if (!write) return;
  void *frames[LIBPLATFORM_BACKTRACE_MAX_FRAMES];
  int count = libplatform_backtrace_capture(frames, LIBPLATFORM_BACKTRACE_MAX_FRAMES);
  char **symbols = libplatform_backtrace_symbolize(frames, count);
  if (!symbols) return;
  for (int i = skip_frames < 0 ? 0 : skip_frames; i < count; ++i) if (symbols[i]) write(symbols[i], context);
  libplatform_backtrace_symbols_free(symbols);
}
