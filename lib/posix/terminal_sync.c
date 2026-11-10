#ifndef _WIN32
#include <pthread.h>
#include <stdbool.h>

static pthread_mutex_t g_terminal_input_mutex = PTHREAD_MUTEX_INITIALIZER;

void terminal_input_lock(void) { (void)pthread_mutex_lock(&g_terminal_input_mutex); }
bool terminal_input_trylock(void) { return pthread_mutex_trylock(&g_terminal_input_mutex) == 0; }
void terminal_input_unlock(void) { (void)pthread_mutex_unlock(&g_terminal_input_mutex); }
#endif
