/** Windows console prompt implementation. */

#ifdef _WIN32

#include <libplatform/question.h>

#include <conio.h>
#include <io.h>
#include <stdio.h>
#include <string.h>

bool platform_is_interactive(void) {
  return _isatty(_fileno(stdin)) != 0;
}

int platform_prompt_question(const char *prompt, char *buffer, size_t max_len, prompt_opts_t opts) {
  if (!prompt || !buffer || max_len < 2 || !platform_is_interactive())
    return -1;

  fputs(prompt, stderr);
  fputc(' ', stderr);
  fflush(stderr);

  size_t length = 0;
  for (;;) {
    int ch = opts.kind == UI_PROMPT_PASSWORD ? _getch() : fgetc(stdin);
    if (ch == EOF || ch == '\n' || ch == '\r')
      break;
    if ((ch == '\b' || ch == 127) && length) {
      --length;
      continue;
    }
    if (length + 1 >= max_len)
      return -1;
    buffer[length++] = (char)ch;
  }
  buffer[length] = '\0';
  fputc('\n', stderr);
  return 0;
}

bool platform_prompt_yes_no(const char *prompt, bool default_yes) {
  return platform_prompt_yes_no_timeout(prompt, default_yes, 30);
}

bool platform_prompt_yes_no_timeout(const char *prompt, bool default_yes, unsigned timeout_seconds) {
  (void)timeout_seconds;
  char response[8] = {0};
  prompt_opts_t opts = PROMPT_OPTS_INLINE;
  if (platform_prompt_question(prompt, response, sizeof(response), opts) != 0 || !response[0])
    return default_yes;
  return response[0] == 'y' || response[0] == 'Y';
}

#endif
