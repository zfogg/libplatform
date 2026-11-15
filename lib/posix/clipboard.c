#include <libplatform/clipboard.h>

int platform_clipboard_read(char *output, size_t capacity) {
  if (!output || capacity == 0)
    return -1;
  output[0] = '\0';
  return -1;
}
