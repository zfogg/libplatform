#include <libplatform/mutex.h>

#ifndef _WIN32
int mutex_init(mutex_t *mutex, const char *name) { (void)name; return pthread_mutex_init(&mutex->impl, NULL); }
int mutex_destroy(mutex_t *mutex) { return pthread_mutex_destroy(&mutex->impl); }
int mutex_lock(mutex_t *mutex) { return pthread_mutex_lock(&mutex->impl); }
int mutex_unlock(mutex_t *mutex) { return pthread_mutex_unlock(&mutex->impl); }
#else
int mutex_init(mutex_t *mutex, const char *name) { (void)name; InitializeCriticalSection(&mutex->impl); return 0; }
int mutex_destroy(mutex_t *mutex) { DeleteCriticalSection(&mutex->impl); return 0; }
int mutex_lock(mutex_t *mutex) { EnterCriticalSection(&mutex->impl); return 0; }
int mutex_unlock(mutex_t *mutex) { LeaveCriticalSection(&mutex->impl); return 0; }
#endif
