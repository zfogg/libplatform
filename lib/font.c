#include <libplatform/font.h>
#include <libplatform/uthash.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct embedded_font { char name[32]; const uint8_t *data; size_t size; UT_hash_handle hh; } embedded_font_t;
static embedded_font_t *g_embedded_fonts;

void platform_font_register(const char *name, const uint8_t *data, size_t size) {
  if (!name || !name[0] || !data || size == 0) return;
  embedded_font_t *font = NULL;
  HASH_FIND_STR(g_embedded_fonts, name, font);
  if (!font) {
    font = calloc(1, sizeof(*font));
    if (!font) return;
    (void)snprintf(font->name, sizeof(font->name), "%s", name);
    HASH_ADD_STR(g_embedded_fonts, name, font);
  }
  font->data = data;
  font->size = size;
}

bool platform_font_get_data(const char *name, const uint8_t **data, size_t *size) {
  embedded_font_t *font = NULL;
  HASH_FIND_STR(g_embedded_fonts, name, font);
  if (data) *data = font ? font->data : NULL;
  if (size) *size = font ? font->size : 0;
  return font != NULL;
}

void platform_font_clear(void) {
  embedded_font_t *font, *tmp;
  HASH_ITER(hh, g_embedded_fonts, font, tmp) {
    HASH_DEL(g_embedded_fonts, font);
    free(font);
  }
}
