#ifndef _WIN32

#include <libplatform/terminal.h>

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

bool terminal_supports_utf8(void) {
  const char *locale = getenv("LC_ALL");
  if (!locale || !*locale) locale = getenv("LC_CTYPE");
  if (!locale || !*locale) locale = getenv("LANG");
  if (!locale || !*locale) return true;
  char normalized[64];
  size_t length = strlen(locale);
  if (length >= sizeof(normalized)) length = sizeof(normalized) - 1;
  for (size_t i = 0; i < length; ++i) normalized[i] = (char)tolower((unsigned char)locale[i]);
  normalized[length] = '\0';
  return strstr(normalized, "utf-8") || strstr(normalized, "utf8");
}

#endif
