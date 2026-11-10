#pragma once
#ifdef _WIN32
#include <winsock2.h>
#endif
#include <ascii-chat/asciichat_errno.h>
#include <stddef.h>

/** Read one copied file or UTF-8 text. Clipboard access is always user initiated. */
asciichat_error_t platform_clipboard_read(char *output, size_t capacity);
