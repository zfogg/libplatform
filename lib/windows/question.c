/** Windows console prompt implementation. */

/* Storage capacities in elements; text buffers include the terminating NUL. */
#define PLATFORM_WINDOWS_QUESTION_TEST_RESPONSE_BUFFER_SIZE 256
#define PLATFORM_WINDOWS_QUESTION_RESPONSE_BUFFER_SIZE 16
#define PLATFORM_WINDOWS_QUESTION_QUESTION_BUFFER_SIZE 4096

/**
 * @file platform/windows/question.c
 * @ingroup platform
 * @brief 💬 Windows interactive prompting with _getch() for secure input
 */

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

  // Consume explicit answers before checking terminal availability.
  {
    char test_password[256];
    const platform_question_callbacks_t *callbacks = platform_question_callbacks();
    if (callbacks->pop_response && callbacks->pop_response(test_password, sizeof(test_password))) {
      size_t len = strlen(test_password);
      if (len >= max_len) {
        return -1; // Password too long
      }
      memcpy(buffer, test_password, len);
      buffer[len] = '\0';
      return 0;
    }
  }

  // Check for non-interactive mode
  if (!platform_is_interactive()) {
    if (platform_question_callbacks()->error) platform_question_callbacks()->error(prompt);
    return -1;
  }

  return platform_question_callbacks()->input ? platform_question_callbacks()->input(prompt, buffer, max_len, opts) : -1;
}

bool platform_prompt_yes_no(const char *prompt, bool default_yes) {
  return platform_prompt_yes_no_timeout(prompt, default_yes, 30);
}

bool platform_prompt_yes_no_timeout(const char *prompt, bool default_yes, unsigned timeout_seconds) {
  if (!prompt) {
    return false;
  }

  // Check for testing environment variable override
  char test_response[PLATFORM_WINDOWS_QUESTION_TEST_RESPONSE_BUFFER_SIZE];
  const platform_question_callbacks_t *callbacks = platform_question_callbacks();
  if (callbacks->pop_response && callbacks->pop_response(test_response, sizeof(test_response))) {
    if (_stricmp(test_response, "yes") == 0 || _stricmp(test_response, "y") == 0) {
      return true;
    } else if (_stricmp(test_response, "no") == 0 || _stricmp(test_response, "n") == 0) {
      return false;
    }
    // Invalid response - fall through to interactive prompt
  }

  char response[PLATFORM_WINDOWS_QUESTION_RESPONSE_BUFFER_SIZE] = {0};
  if (platform_is_interactive()) {
    char question[PLATFORM_WINDOWS_QUESTION_QUESTION_BUFFER_SIZE];
    snprintf(question, sizeof(question), "%s %s", prompt, default_yes ? "(Y/n)?" : "(y/N)?");
    prompt_opts_t opts = PROMPT_OPTS_INLINE;
    opts.kind = callbacks->notice_kind ? callbacks->notice_kind() : UI_PROMPT_CONFIRMATION;
    opts.timeout_seconds = timeout_seconds;
    if (!callbacks->input || callbacks->input(question, response, sizeof(response), opts) != 0)
      return false;
  } else {
    if (callbacks->error) callbacks->error(prompt);
    return false;
  }
  if (_stricmp(response, "yes") == 0 || _stricmp(response, "y") == 0) {
    if (callbacks->usage) callbacks->usage(callbacks->kind_name && callbacks->notice_kind ? callbacks->kind_name(callbacks->notice_kind()) : "confirmation", "accepted");
    return true;
  }
  if (_stricmp(response, "no") == 0 || _stricmp(response, "n") == 0) {
    if (callbacks->usage) callbacks->usage(callbacks->kind_name && callbacks->notice_kind ? callbacks->kind_name(callbacks->notice_kind()) : "confirmation", "declined");
    return false;
  }
  if (callbacks->usage) callbacks->usage(callbacks->kind_name && callbacks->notice_kind ? callbacks->kind_name(callbacks->notice_kind()) : "confirmation", default_yes ? "accepted" : "declined");
  return default_yes;
}

