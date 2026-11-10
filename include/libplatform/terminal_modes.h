#pragma once

typedef enum terminal_color_mode {
  TERM_COLOR_AUTO = -1,
  TERM_COLOR_NONE = 0,
  TERM_COLOR_16 = 1,
  TERM_COLOR_256 = 2,
  TERM_COLOR_TRUECOLOR = 3
} terminal_color_mode_t;

typedef enum render_mode {
  RENDER_MODE_FOREGROUND = 0,
  RENDER_MODE_BACKGROUND = 1,
  RENDER_MODE_HALF_BLOCK = 2
} render_mode_t;
