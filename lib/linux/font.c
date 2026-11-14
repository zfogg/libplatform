/**
 * @file platform/linux/font.c
 * @ingroup platform
 * @brief Font resolution for Linux: fontconfig name → absolute .ttf path
 */
#if defined(__linux__) || defined(__FreeBSD__) || defined(__OpenBSD__)
#include <libplatform/font.h>

#ifndef USE_MUSL
#include <fontconfig/fontconfig.h>
#endif
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <stddef.h>


// Fallback font name for fontconfig when a requested font is not found.
// DejaVu Sans Mono ships with the vast majority of distros.
static const char *k_fallback = "DejaVu Sans Mono";

static bool is_absolute_path(const char *s) {
  return s && s[0] == '/';
}
static bool file_exists(const char *p) {
  struct stat st;
  return stat(p, &st) == 0;
}

#ifndef USE_MUSL
static int resolve_via_fontconfig(const char *name, char *out, size_t sz) {
  FcConfig *fc = FcInitLoadConfigAndFonts();
  FcPattern *pat = FcNameParse((const FcChar8 *)name);
  if (!pat)
    return (-1);

  FcConfigSubstitute(fc, pat, FcMatchPattern);
  FcDefaultSubstitute(pat);

  FcResult result;
  FcPattern *match = FcFontMatch(fc, pat, &result);
  FcPatternDestroy(pat);

  if (!match || result != FcResultMatch)
    return (-1);

  FcChar8 *file = NULL;
  if (FcPatternGetString(match, FC_FILE, 0, &file) != FcResultMatch || !file) {
    FcPatternDestroy(match);
    return (-1);
  }
  snprintf(out, sz, "%s", (const char *)file);
  FcPatternDestroy(match);
  return 0;
}
#endif
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
      return (-1);
    snprintf(out, out_size, "%s", eff);
    *out_is_path = true;
    return file_exists(out) ? 0 : (-1);
  }

  // Check for bundled font names ("matrix" or "default")
  if (strcmp(eff, "matrix") == 0) {
    if (out_font_data)
      *out_font_data = matrix_data;
    if (out_font_data_size)
      *out_font_data_size = matrix_size;
    snprintf(out, out_size, "matrix");
    *out_is_path = false;
    return 0;
  }

  if (strcmp(eff, "default") == 0) {
    if (out_font_data)
      *out_font_data = default_data;
    if (out_font_data_size)
      *out_font_data_size = default_size;
    snprintf(out, out_size, "default");
    *out_is_path = false;
    return 0;
  }

#ifndef USE_MUSL
  *out_is_path = true;
  int err = resolve_via_fontconfig(eff, out, out_size);
  if (err != 0 && strcmp(eff, k_fallback) != 0) {
    err = resolve_via_fontconfig(k_fallback, out, out_size);
  }

  // Fallback chain: system fonts → bundled default font
  if (err != 0) {
    if (out_font_data)
      *out_font_data = default_data;
    if (out_font_data_size)
      *out_font_data_size = default_size;
    out[0] = '\0';
    *out_is_path = false;
    err = 0;
  }

  return err;
#else
  // musl static builds: fontconfig not available, use bundled default font
  if (out_font_data)
    *out_font_data = default_data;
  if (out_font_data_size)
    *out_font_data_size = default_size;
  out[0] = '\0';
  *out_is_path = false;
  return 0;
#endif
}
#endif

