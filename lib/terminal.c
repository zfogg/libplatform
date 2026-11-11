/**
 * @file platform/terminal.c
 * @ingroup platform
 * @brief Cross-platform unified color detection and terminal capabilities
 */

#include <libplatform/terminal.h>
#include <libplatform/system.h>
#include <libplatform/system.h>
#ifndef _WIN32
#include <unistd.h>
#include <pthread.h>
#else
#include <io.h>
#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2
#endif
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

int terminal_format_hyperlink(char *buffer, size_t capacity, const char *url, const char *label, bool enabled) {
  if (!buffer || !capacity || !url || !*url || !label)
    return -1;
  buffer[0] = '\0';
  const char *parts[] = {url, label};
  for (size_t i = 0; i < 2; ++i) {
    for (const unsigned char *p = (const unsigned char *)parts[i]; *p; ++p) {
      if (*p < 32 || *p == 127)
        return -1;
    }
  }
  int length = enabled ? snprintf(buffer, capacity, "\033]8;;%s\033\\\033[4m%s\033[24m\033]8;;\033\\", url, label)
                       : snprintf(buffer, capacity, "%s", label);
  if (length < 0 || (size_t)length >= capacity) {
    buffer[0] = '\0';
    return -1;
  }
  return length;
}

bool terminal_should_use_utf8(terminal_utf8_setting_t user_setting) {
  return user_setting == TERMINAL_UTF8_SETTING_TRUE ||
         (user_setting == TERMINAL_UTF8_SETTING_AUTO && terminal_supports_utf8());
}

int terminal_resize_poll(terminal_resize_watcher_t *watcher, int fd, bool *changed) {
  if (changed)
    *changed = false;
  if (!watcher || !changed)
    return -1;
  terminal_size_t size;
  int result = terminal_get_size_fd(fd, &size);
  if (result != 0)
    return result;
  if (size.cols <= 0 || size.rows <= 0)
    return -1;
  *changed = size.cols != watcher->size.cols || size.rows != watcher->size.rows;
  watcher->size = size;
  return 0;
}

/**
 * @brief Determine if color output should be used
 *
 * Priority order:
 * 1. If --color flag is set → ALWAYS use colors (force override)
 * 2. If CLAUDECODE env var is set → NEVER use colors (LLM automation)
 * 3. If output is not a TTY (piping) → NO colors
 * 4. If --color-mode=none → NO colors (user choice)
 * 5. Otherwise → Use colors
 *
 * @param fd File descriptor to check (STDOUT_FILENO or STDERR_FILENO)
 * @return true if colors should be used, false otherwise
 */
bool terminal_should_color_output(int fd, const terminal_options_t *options) {
  if (!options) {
    return false;
  }
  // Check color_mode setting - if explicitly set to NONE, disable colors
  terminal_color_mode_t color_mode = options->color_mode;
  if (color_mode == TERM_COLOR_NONE) {
    return false; // Color mode explicitly disabled
  }

  // Get current color setting (auto/true/false)
  int color_setting = options->color;

  // Priority 1: --color=true - Force colors ON (overrides everything)
  if (color_setting == TERMINAL_COLOR_SETTING_TRUE) {
    return true; // ALWAYS colorize
  }

  // Priority 2: --color=false - Force colors OFF (overrides everything)
  if (color_setting == TERMINAL_COLOR_SETTING_FALSE) {
    return false; // NEVER colorize
  }

  // Auto mode follows the destination terminal. Application-specific flags
  // and environment policy belong to the application, not libplatform.
  return platform_isatty(fd);
}

/**
 * @brief Get current color mode considering all overrides
 *
 * Determines effective color mode by checking:
 * 1. --color flag (force enable)
 * 2. --color-mode option (none/16/256/truecolor)
 * 3. Terminal capability detection
 *
 * @return Effective terminal_color_mode_t to use
 */
terminal_color_mode_t terminal_get_effective_color_mode(const terminal_options_t *options) {
  // If colors are disabled entirely by terminal_should_color_output(), return NONE
  if (!options || !terminal_should_color_output(STDOUT_FILENO, options)) {
    return TERM_COLOR_NONE;
  }

  // If --color-mode is set to specific value, use it
  terminal_color_mode_t color_mode = options ? options->color_mode : TERM_COLOR_AUTO;
  if (color_mode != TERM_COLOR_AUTO) {
    return color_mode;
  }

  // Otherwise auto-detect from terminal capabilities
  // For now, return the detected color mode from terminal capabilities
  // This will be used by apply_color_mode_override() if needed
  return TERM_COLOR_AUTO; // Let the calling code handle auto-detection
}

/* ============================================================================
 * Interactive Mode and TTY State Detection
 * ============================================================================ */

bool terminal_is_stdin_tty(void) {
  return platform_isatty(STDIN_FILENO) != 0;
}

bool terminal_is_stdout_tty(void) {
  return platform_isatty(STDOUT_FILENO) != 0;
}

bool terminal_is_stderr_tty(void) {
  return platform_isatty(STDERR_FILENO) != 0;
}

bool terminal_is_interactive(void) {
  return terminal_is_stdin_tty() && terminal_is_stdout_tty();
}

/**
 * @brief Get effective terminal width with fallback priority
 *
 * Returns width in priority order:
 * 1. Option width if set to non-zero value
 * 2. Detected terminal width
 * 3. OPT_WIDTH_DEFAULT fallback
 */
unsigned short int terminal_get_effective_width(int width, unsigned short fallback_width) {
  // If width option is explicitly set to non-zero, use it
  if (width > 0) {
    return (unsigned short int)width;
  }

  // Try to detect actual terminal width
  terminal_size_t size;
  if (terminal_get_size(&size) == 0 && size.cols > 0) {
    return (unsigned short int)size.cols;
  }

  // Fallback to default
  return fallback_width;
}

/**
 * @brief Get effective terminal height with fallback priority
 *
 * Returns height in priority order:
 * 1. Option height if set to non-zero value
 * 2. Detected terminal height
 * 3. OPT_HEIGHT_DEFAULT fallback
 */
unsigned short int terminal_get_effective_height(int height, unsigned short fallback_height) {
  // If height option is explicitly set to non-zero, use it
  if (height > 0) {
    return (unsigned short int)height;
  }

  // Try to detect actual terminal height
  terminal_size_t size;
  if (terminal_get_size(&size) == 0 && size.rows > 0) {
    return (unsigned short int)size.rows;
  }

  // Fallback to default
  return fallback_height;
}

bool terminal_is_piped_output(void) {
  return !terminal_is_stdout_tty();
}

bool terminal_should_force_stderr(void) {
  if (!terminal_is_piped_output())
    return false;
  const char *testing = platform_getenv("TESTING");
  return !(testing && testing[0] == '1' && testing[1] == '\0');
}

bool terminal_can_prompt_user(bool user_setting) {
  // Must be fully interactive (stdin and stdout are TTYs)
  if (!terminal_is_interactive()) {
    return false;
  }

  // Must not be in snapshot mode (non-interactive capture)
  if (!user_setting) {
    return false;
  }

  // Must not have automated prompt responses configured
  const char *auto_response = platform_getenv("ASCII_CHAT_QUESTION_PROMPT_RESPONSE");
  if (auto_response && *auto_response != '\0') {
    // NOLINTNEXTLINE(readability-simplify-boolean-expr) - Intentional check for configured automated responses
    return false;
  }

  // All checks passed, interactive prompts are appropriate
  return true;
}

/* ============================================================================
 * Theme-Aware Renderer Color Selection
 * ============================================================================ */

/**
 * @brief Get theme-aware default foreground color for the renderer
 * @param theme Terminal theme (dark or light)
 * @param out_r Pointer to store red component (0-255)
 * @param out_g Pointer to store green component (0-255)
 * @param out_b Pointer to store blue component (0-255)
 *
 * Returns appropriate default text color based on terminal theme.
 * Both Linux and macOS renderers use this for consistent color selection.
 */
void terminal_get_default_foreground_color(int theme, uint8_t *out_r, uint8_t *out_g, uint8_t *out_b) {
  if (theme == 1) { // TERM_RENDERER_THEME_LIGHT
    *out_r = TERMINAL_COLOR_THEME_LIGHT_FG_R;
    *out_g = TERMINAL_COLOR_THEME_LIGHT_FG_G;
    *out_b = TERMINAL_COLOR_THEME_LIGHT_FG_B;
  } else { // TERM_RENDERER_THEME_DARK or TERM_RENDERER_THEME_AUTO
    *out_r = TERMINAL_COLOR_THEME_DARK_FG_R;
    *out_g = TERMINAL_COLOR_THEME_DARK_FG_G;
    *out_b = TERMINAL_COLOR_THEME_DARK_FG_B;
  }
}

/**
 * @brief Get theme-aware default background color for the renderer
 * @param theme Terminal theme (dark or light)
 * @param out_r Pointer to store red component (0-255)
 * @param out_g Pointer to store green component (0-255)
 * @param out_b Pointer to store blue component (0-255)
 *
 * Returns appropriate default background color based on terminal theme.
 * Light theme uses white background, dark theme uses black background.
 */
void terminal_get_default_background_color(int theme, uint8_t *out_r, uint8_t *out_g, uint8_t *out_b) {
  if (theme == 1) { // TERM_RENDERER_THEME_LIGHT
    *out_r = TERMINAL_COLOR_THEME_LIGHT_BG_R;
    *out_g = TERMINAL_COLOR_THEME_LIGHT_BG_G;
    *out_b = TERMINAL_COLOR_THEME_LIGHT_BG_B;
  } else { // TERM_RENDERER_THEME_DARK or TERM_RENDERER_THEME_AUTO
    *out_r = TERMINAL_COLOR_THEME_DARK_BG_R;
    *out_g = TERMINAL_COLOR_THEME_DARK_BG_G;
    *out_b = TERMINAL_COLOR_THEME_DARK_BG_B;
  }
}

/**
 * @brief Check if terminal control sequences should be used for the given fd
 * @param fd File descriptor to check
 * @return true if terminal control sequences should be used, false otherwise
 *
 * This function determines whether to send terminal POSITIONING and CONTROL
 * sequences (cursor home, clear screen, hide cursor, etc.) which should only
 * be sent to TTY and never to pipes/redirected output.
 *
 * Note: This does NOT control ANSI COLOR CODES, which are controlled by
 * --color-mode option and may be sent to pipes if explicitly requested.
 */
bool terminal_should_use_control_sequences(int fd, bool user_setting) {
  if (fd < 0) {
    return false;
  }
  if (user_setting) {
    return false;
  }
  const char *testing_env = platform_getenv("TESTING");
  if (testing_env != NULL) {
    return false;
  }
  return platform_isatty(fd) != 0;
}

#ifndef _WIN32
static pthread_mutex_t g_terminal_input_mutex = PTHREAD_MUTEX_INITIALIZER;

void terminal_input_lock(void) { (void)pthread_mutex_lock(&g_terminal_input_mutex); }

bool terminal_input_trylock(void) { return pthread_mutex_trylock(&g_terminal_input_mutex) == 0; }

void terminal_input_unlock(void) { (void)pthread_mutex_unlock(&g_terminal_input_mutex); }

bool terminal_supports_utf8(void) {
  const char *locale = platform_getenv("LC_ALL");
  if (!locale || !*locale) locale = platform_getenv("LC_CTYPE");
  if (!locale || !*locale) locale = platform_getenv("LANG");
  if (!locale || !*locale) return true;

  char normalized[64];
  size_t length = strlen(locale);
  if (length >= sizeof(normalized)) length = sizeof(normalized) - 1;
  for (size_t i = 0; i < length; ++i) normalized[i] = (char)tolower((unsigned char)locale[i]);
  normalized[length] = '\0';
  return strstr(normalized, "utf-8") || strstr(normalized, "utf8");
}
#endif
