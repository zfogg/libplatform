/**
 * @file platform/macos/font.c
 * @ingroup platform
 * @brief Font resolution for macOS: file paths, bundled fonts, and unresolved name passthrough
 */
#ifdef __APPLE__

/* Storage capacities in elements; text buffers include the terminating NUL. */

#include <libplatform/font.h>

#include <libplatform/filesystem.h>
#ifndef LIBPLATFORM_STANDALONE
#include <ascii-chat/asciichat_errno.h>
#include <ascii-chat/log/log.h>
#else
#define ASCIICHAT_OK 0
#define ERROR_INVALID_PARAM 1
#define ERROR_NOT_FOUND 2
#define SET_ERRNO(...) (-1)
#define log_debug(...) ((void)0)
#endif
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static bool is_absolute_path(const char *s) {
  return s && s[0] == '/';
}
static bool file_exists(const char *p) {
  struct stat st;
  return stat(p, &st) == 0;
}

int platform_font_resolve(const char *spec, char *out, size_t out_size, bool *out_is_path,
                          const uint8_t **out_font_data, size_t *out_font_data_size) {
  // If no font is specified, use the bundled default font (DejaVu Sans Mono) for render-file output.
  // This ensures render-file always has a suitable fallback font available.
  const char *eff = (spec && spec[0]) ? spec : "default";
  const uint8_t *matrix_data = NULL, *default_data = NULL;
  size_t matrix_size = 0, default_size = 0;
  platform_font_get_data("matrix", &matrix_data, &matrix_size);
  platform_font_get_data("default", &default_data, &default_size);

  if (out_font_data)
    *out_font_data = NULL;
  if (out_font_data_size)
    *out_font_data_size = 0;

  if (is_absolute_path(eff) || strchr(eff, '/') ||
      (strcmp(eff, "default") != 0 && strcmp(eff, "matrix") != 0 && file_exists(eff))) {
    if (strlen(eff) >= out_size)
      return SET_ERRNO(ERROR_INVALID_PARAM, "render-font: path exceeds output capacity");
    snprintf(out, out_size, "%s", eff);
    *out_is_path = true;
    return file_exists(out) ? ASCIICHAT_OK : SET_ERRNO(ERROR_NOT_FOUND, "render-font: not found: %s", out);
  }

  // Check for bundled font names ("matrix" or "default")
  if (strcmp(eff, "matrix") == 0) {
    if (out_font_data)
      *out_font_data = matrix_data;
    if (out_font_data_size)
      *out_font_data_size = matrix_size;
    // Also provide a temporary file path for the bundled font.
    static char tmp_path[PLATFORM_MAX_PATH_LENGTH] = {0};
    if (tmp_path[0] == '\0' && out_font_data && *out_font_data) {
      strlcpy(tmp_path, "/tmp/ascii-chat-matrix-XXXXXX.ttf", sizeof(tmp_path));
      int fd = mkstemps(tmp_path, 4); // 4 = ".ttf" suffix
      if (fd >= 0) {
        write(fd, *out_font_data, *out_font_data_size);
        close(fd);
        log_debug("wrote bundled matrix font to %s", tmp_path);
      }
    }
    if (tmp_path[0] != '\0') {
      snprintf(out, out_size, "%s", tmp_path);
      *out_is_path = true;
    }
    return ASCIICHAT_OK;
  }

  if (strcmp(eff, "default") == 0) {
    if (out_font_data)
      *out_font_data = default_data;
    if (out_font_data_size)
      *out_font_data_size = default_size;
    // Also provide a temporary file path for the bundled font.
    static char tmp_path_default[PLATFORM_MAX_PATH_LENGTH] = {0};
    if (tmp_path_default[0] == '\0' && out_font_data && *out_font_data) {
      strlcpy(tmp_path_default, "/tmp/ascii-chat-default-XXXXXX.ttf", sizeof(tmp_path_default));
      int fd = mkstemps(tmp_path_default, 4); // 4 = ".ttf" suffix
      if (fd >= 0) {
        write(fd, *out_font_data, *out_font_data_size);
        close(fd);
        log_debug("wrote bundled default font to %s", tmp_path_default);
      }
    }
    if (tmp_path_default[0] != '\0') {
      snprintf(out, out_size, "%s", tmp_path_default);
      *out_is_path = true;
    }
    return ASCIICHAT_OK;
  }

  // Pass other specifications through unchanged; this does not resolve family names.
  // The FreeType renderer treats this string as a file path and reports load failures.
  snprintf(out, out_size, "%s", eff);
  *out_is_path = false;

  log_debug("passing unresolved font specification '%s' to the FreeType renderer", eff);
  return ASCIICHAT_OK;
}
#endif // __APPLE__
