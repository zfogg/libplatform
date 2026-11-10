#include <libplatform/question.h>
#include <string.h>

static platform_question_callbacks_t g_callbacks;

void platform_question_set_callbacks(const platform_question_callbacks_t *callbacks) {
  if (callbacks)
    g_callbacks = *callbacks;
  else
    memset(&g_callbacks, 0, sizeof(g_callbacks));
}

const platform_question_callbacks_t *platform_question_callbacks(void) {
  return &g_callbacks;
}
