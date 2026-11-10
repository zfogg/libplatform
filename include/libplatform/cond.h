#pragma once

#include <libplatform/mutex.h>

#ifdef _WIN32
#include <windows.h>
typedef struct cond_t { CONDITION_VARIABLE impl; } cond_t;
#else
#include <pthread.h>
typedef struct cond_t { pthread_cond_t impl; } cond_t;
#endif

int cond_init(cond_t *cond, const char *name);
int cond_destroy(cond_t *cond);
int cond_wait(cond_t *cond, mutex_t *mutex);
int cond_signal(cond_t *cond);
int cond_broadcast(cond_t *cond);
