#include <libplatform/clipboard.h>
#include <windows.h>
#include <shellapi.h>
#include <limits.h>
#include <stdlib.h>
#include <libplatform/util/string.h>

static platform_clipboard_format_paths_fn g_path_formatter;

void platform_clipboard_set_path_formatter(platform_clipboard_format_paths_fn formatter) {
  g_path_formatter = formatter;
}

int platform_clipboard_read(char *output, size_t capacity) {
  if (!output || capacity < 2 || capacity > INT_MAX || !OpenClipboard(NULL))
    return -1;
  output[0] = '\0';
  int result = -1;
  if (IsClipboardFormatAvailable(CF_HDROP)) {
    HDROP files = (HDROP)GetClipboardData(CF_HDROP);
    UINT count = files ? DragQueryFileW(files, 0xffffffff, NULL, 0) : 0;
    char **paths = count ? calloc(count, sizeof(*paths)) : NULL;
    if (paths) {
      result = 0;
      for (UINT i = 0; i < count; ++i) {
        UINT length = DragQueryFileW(files, i, NULL, 0);
        wchar_t *wide = calloc((size_t)length + 1, sizeof(*wide));
        char utf8[4096];
        if (!wide || !DragQueryFileW(files, i, wide, length + 1) ||
            !WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, wide, -1, utf8, sizeof(utf8), NULL, NULL)) {
          free(wide);
          result = -1;
          break;
        }
        paths[i] = platform_strdup(utf8);
        free(wide);
        if (!paths[i]) {
          result = -1;
          break;
        }
      }
      if (result == 0 && g_path_formatter)
        result = g_path_formatter((const char *const *)paths, count, output, capacity);
      for (UINT i = 0; i < count; ++i)
        free(paths[i]);
      free(paths);
    }
  } else {
    HANDLE data = GetClipboardData(CF_UNICODETEXT);
    const wchar_t *text = data ? GlobalLock(data) : NULL;
    if (text && WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, text, -1, output, (int)capacity, NULL, NULL))
      result = 0;
    if (data)
      GlobalUnlock(data);
  }
  CloseClipboard();
  return result;
}
