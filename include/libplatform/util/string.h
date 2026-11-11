#pragma once

#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdbool.h>

#ifdef _WIN32
#include <BaseTsd.h>
typedef SSIZE_T ssize_t;
#else
#include <sys/types.h>
#endif

int platform_vsnprintf(char *str, size_t size, const char *format, va_list ap);
int platform_vasprintf(char **strp, const char *format, va_list ap);
int platform_asprintf(char **strp, const char *format, ...);
ssize_t platform_getline(char **lineptr, size_t *n, FILE *stream);
char *platform_strdup(const char *s);
int platform_strcasecmp(const char *s1, const char *s2);
int platform_strncasecmp(const char *s1, const char *s2, size_t n);
size_t platform_strlcpy(char *dst, const char *src, size_t size);
int platform_strncpy(char *dst, size_t dst_size, const char *src, size_t count);
char *platform_strtok_r(char *str, const char *delim, char **saveptr);

typedef enum platform_color {
  PLATFORM_COLOR_GREY,
  PLATFORM_COLOR_FATAL,
  PLATFORM_COLOR_DEBUG,
  PLATFORM_COLOR_INFO,
  PLATFORM_COLOR_ERROR,
  PLATFORM_COLOR_WARN,
  PLATFORM_COLOR_DEV,
  PLATFORM_COLOR_RESET,
  PLATFORM_COLOR_COUNT
} platform_color_t;

void platform_color_set_enabled(bool enabled);
void platform_color_set_truecolor_supported(bool supported);
void platform_color_set_code(platform_color_t color, const char *ansi_code);
const char *platform_colored_string(bool enabled, platform_color_t color, const char *text);
