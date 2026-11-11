/**
 * @file util/string.c
 * @ingroup util
 * @brief 🔤 String manipulation utilities: ASCII escaping, trimming, case conversion, and formatting
 */

#include <libplatform/util/string.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

static bool g_platform_colors_enabled = true;
static char g_platform_color_codes[PLATFORM_COLOR_COUNT][24] = {
    "\033[90m", "\033[1;31m", "\033[36m", "\033[32m",
    "\033[31m", "\033[33m", "\033[35m", "\033[0m"};
static _Thread_local char g_platform_colored_buffers[4][512];
static _Thread_local unsigned g_platform_colored_buffer_index;
static bool g_platform_colors_env_checked;

static void platform_color_read_environment(void) {
  if (g_platform_colors_env_checked) return;
  g_platform_colors_env_checked = true;
  const char *enabled = getenv("LIBPLATFORM_COLOR");
  if (enabled && (!strcmp(enabled, "0") || !strcmp(enabled, "false") || !strcmp(enabled, "no")))
    g_platform_colors_enabled = false;
  const char *error_code = getenv("LIBPLATFORM_COLOR_ERROR");
  if (error_code) platform_color_set_code(PLATFORM_COLOR_ERROR, error_code);
  const char *warn_code = getenv("LIBPLATFORM_COLOR_WARN");
  if (warn_code) platform_color_set_code(PLATFORM_COLOR_WARN, warn_code);
}

void platform_color_set_enabled(bool enabled) { g_platform_colors_enabled = enabled; }

void platform_color_set_code(platform_color_t color, const char *ansi_code) {
  if (color < 0 || color >= PLATFORM_COLOR_COUNT || !ansi_code) return;
  (void)snprintf(g_platform_color_codes[color], sizeof(g_platform_color_codes[color]), "%s", ansi_code);
}

const char *platform_colored_string(bool enabled, platform_color_t color, const char *text) {
  if (!text) return "";
  platform_color_read_environment();
  if (!enabled || !g_platform_colors_enabled || color < 0 || color >= PLATFORM_COLOR_COUNT) return text;
  char *buffer = g_platform_colored_buffers[g_platform_colored_buffer_index++ % 4];
  (void)snprintf(buffer, sizeof(g_platform_colored_buffers[0]), "%s%s%s", g_platform_color_codes[color], text,
                 g_platform_color_codes[PLATFORM_COLOR_RESET]);
  return buffer;
}

void escape_ascii(const char *str, const char *escape_char, char *out_buffer, size_t out_buffer_size) {
  if (escape_char == NULL || str == NULL || !out_buffer || out_buffer_size == 0) {
    return;
  }

  // Count how many characters need escaping
  size_t escape_count = 0;
  for (size_t i = 0; i < strlen(str); i++) {
    for (size_t j = 0; j < strlen(escape_char); j++) {
      if (str[i] == escape_char[j]) {
        escape_count++;
        break;
      }
    }
  }

  size_t total_len = strlen(str) + escape_count;
  if (out_buffer_size < total_len) {
    return;
  }

  size_t out_pos = 0;
  for (size_t i = 0; i < strlen(str); i++) {
    bool needs_escape = false;
    for (size_t j = 0; j < strlen(escape_char); j++) {
      if (str[i] == escape_char[j]) {
        needs_escape = true;
        break;
      }
    }

    if (needs_escape) {
      out_buffer[out_pos++] = '\\';
    }
    out_buffer[out_pos++] = str[i];
  }

  out_buffer[out_pos] = '\0';
}

bool validate_shell_safe(const char *str, const char *allowed_chars) {
  if (!str) {
    return false;
  }

  // Shell metacharacters that are dangerous
  // ; & | $ ` \ " ' < > ( ) [ ] { } * ? ! ~ # @ space tab newline
  const char *dangerous = ";|$`\\\"'<>()[]{}*?!~#@ \t\n\r";

  for (const unsigned char *p = (const unsigned char *)str; *p; p++) {
    unsigned char c = *p;
    if (c < 128) {

      // Allow ASCII alphanumeric characters
      if (isalnum(c)) {
        continue;
      }

      // Check if character is in allowed_chars list
      if (allowed_chars) {
        bool is_allowed = false;
        for (size_t j = 0; allowed_chars[j] != '\0'; j++) {
          if (c == (unsigned char)allowed_chars[j]) {
            is_allowed = true;
            break;
          }
        }
        if (is_allowed) {
          continue;
        }
      }

      // Check if character is dangerous
      bool is_dangerous = false;
      for (size_t j = 0; dangerous[j] != '\0'; j++) {
        if (c == (unsigned char)dangerous[j]) {
          is_dangerous = true;
          break;
        }
      }
      if (is_dangerous) {
        return false;
      }
    }

  }

  return true;
}

bool escape_shell_single_quotes(const char *str, char *out_buffer, size_t out_buffer_size) {
  if (!str || !out_buffer || out_buffer_size == 0) {
    return false;
  }

  // Calculate required size: each ' becomes '\'' (4 chars) instead of 1
  // Worst case: all chars are quotes, so we need strlen(str) * 4 + 2 (for surrounding quotes)
  size_t required_size = 2; // Start and end quotes
  for (size_t i = 0; str[i] != '\0'; i++) {
    if (str[i] == '\'') {
      required_size += 4; // '\''
    } else {
      required_size += 1;
    }
  }

  if (out_buffer_size < required_size) {
    return false;
  }

  size_t out_pos = 0;
  out_buffer[out_pos++] = '\''; // Opening quote

  for (size_t i = 0; str[i] != '\0'; i++) {
    if (str[i] == '\'') {
      // Replace ' with '\'' (close quote, escaped quote, open quote)
      out_buffer[out_pos++] = '\'';
      out_buffer[out_pos++] = '\\';
      out_buffer[out_pos++] = '\'';
      out_buffer[out_pos++] = '\'';
    } else {
      out_buffer[out_pos++] = str[i];
    }
  }

  out_buffer[out_pos++] = '\''; // Closing quote
  out_buffer[out_pos] = '\0';

  return true;
}

bool escape_shell_double_quotes(const char *str, char *out_buffer, size_t out_buffer_size) {
  if (!str || !out_buffer || out_buffer_size == 0) {
    return false;
  }

  // Calculate required size: each " becomes \" (2 chars) instead of 1
  // Each \ also needs escaping, so \\ becomes \\\\
  // Worst case: all chars are quotes/backslashes, so we need strlen(str) * 2 + 2 (for surrounding quotes)
  size_t required_size = 2; // Start and end quotes
  for (size_t i = 0; str[i] != '\0'; i++) {
    if (str[i] == '"' || str[i] == '\\') {
      required_size += 2; // \" or '\\'
    } else {
      required_size += 1;
    }
  }

  if (out_buffer_size < required_size) {
    return false;
  }

  size_t out_pos = 0;
  out_buffer[out_pos++] = '"'; // Opening quote

  for (size_t i = 0; str[i] != '\0'; i++) {
    if (str[i] == '"') {
      // Escape double quote: " becomes \"
      out_buffer[out_pos++] = '\\';
      out_buffer[out_pos++] = '"';
    } else if (str[i] == '\\') {
      // Escape backslash: \ becomes '\\'
      out_buffer[out_pos++] = '\\';
      out_buffer[out_pos++] = '\\';
    } else {
      out_buffer[out_pos++] = str[i];
    }
  }

  out_buffer[out_pos++] = '"'; // Closing quote
  out_buffer[out_pos] = '\0';

  return true;
}

bool string_needs_shell_quoting(const char *str) {
  if (!str || str[0] == '\0') {
    return false; // Empty strings don't need quoting
  }

  // Check for characters that need shell quoting
  for (size_t i = 0; str[i] != '\0'; i++) {
    char c = str[i];

    // Whitespace characters always need quoting
    if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
      return true;
    }

    // Shell special characters
    if (c == '"' || c == '\'' || c == '$' || c == '`' || c == '\\' || c == '<' || c == '>' || c == '&' || c == ';' ||
        c == '|' || c == '(' || c == ')' || c == '[' || c == ']' || c == '{' || c == '}' || c == '*' || c == '?' ||
        c == '!' || c == '~' || c == '#' || c == '@') {
      return true;
    }
  }

  return false;
}

bool escape_path_for_shell(const char *path, char *out_buffer, size_t out_buffer_size) {
  if (!path || !out_buffer || out_buffer_size == 0) {
    return false;
  }

  // Check if quoting is needed
  if (!string_needs_shell_quoting(path)) {
    // No quoting needed, just copy the path
    size_t path_len = strlen(path);
    if (out_buffer_size <= path_len) {
      return false;
    }
    memcpy(out_buffer, path, path_len + 1);
    return true;
  }

  // Quoting is needed, use platform-specific escaping
#ifdef _WIN32
  return escape_shell_double_quotes(path, out_buffer, out_buffer_size);
#else
  return escape_shell_single_quotes(path, out_buffer, out_buffer_size);
#endif
}

