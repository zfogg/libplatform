#include <libplatform/util/utf8.h>
#include <utf8proc.h>

bool utf8_is_valid(const char *text) {
  if (text == NULL)
    return false;
  const utf8proc_uint8_t *p = (const utf8proc_uint8_t *)text;
  while (*p != 0) {
    utf8proc_int32_t codepoint;
    utf8proc_ssize_t length = utf8proc_iterate(p, -1, &codepoint);
    if (length < 1)
      return false;
    p += length;
  }
  return true;
}
