#pragma once

#include <stdbool.h>
#ifdef _WIN32
#include <windows.h>
typedef struct mutex_t { CRITICAL_SECTION impl; } mutex_t;
#else
#include <pthread.h>
typedef struct mutex_t { pthread_mutex_t impl; } mutex_t;
#endif

int mutex_init(mutex_t *mutex, const char *name);
int mutex_destroy(mutex_t *mutex);
int mutex_lock(mutex_t *mutex);
int mutex_unlock(mutex_t *mutex);
