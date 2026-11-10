#ifdef _WIN32

#include <libplatform/terminal.h>
#include <windows.h>

bool terminal_supports_utf8(void) {
  return GetConsoleOutputCP() == CP_UTF8 || GetConsoleCP() == CP_UTF8;
}

#endif
