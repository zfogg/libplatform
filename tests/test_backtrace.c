#include <libplatform/backtrace.h>
#include <assert.h>
#include <stdio.h>

static void sink(const char *line, void *context) { (void)context; assert(line && line[0]); }
static int nested(void) { void *frames[8]; int n = libplatform_backtrace_capture(frames, 8); assert(n >= 0); if (n) { char **s = libplatform_backtrace_symbolize(frames, n); assert(s); libplatform_backtrace_symbols_free(s); } libplatform_backtrace_print(0, sink, NULL); return n; }
int main(void) { (void)nested(); puts("libplatform backtrace ok"); return 0; }
