#include <libplatform/cond.h>

#ifndef _WIN32
int cond_init(cond_t *cond, const char *name) { (void)name; return pthread_cond_init(&cond->impl, NULL); }
int cond_destroy(cond_t *cond) { return pthread_cond_destroy(&cond->impl); }
int cond_wait(cond_t *cond, mutex_t *mutex) { return pthread_cond_wait(&cond->impl, &mutex->impl); }
int cond_signal(cond_t *cond) { return pthread_cond_signal(&cond->impl); }
int cond_broadcast(cond_t *cond) { return pthread_cond_broadcast(&cond->impl); }
#else
int cond_init(cond_t *cond, const char *name) { (void)name; InitializeConditionVariable(&cond->impl); return 0; }
int cond_destroy(cond_t *cond) { (void)cond; return 0; }
int cond_wait(cond_t *cond, mutex_t *mutex) { return SleepConditionVariableCS(&cond->impl, &mutex->impl, INFINITE) ? 0 : (int)GetLastError(); }
int cond_signal(cond_t *cond) { WakeConditionVariable(&cond->impl); return 0; }
int cond_broadcast(cond_t *cond) { WakeAllConditionVariable(&cond->impl); return 0; }
#endif
