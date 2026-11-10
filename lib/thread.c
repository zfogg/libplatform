#include <libplatform/thread.h>
#ifdef _WIN32
#include <windows.h>
uint64_t thread_current_id(void) { return (uint64_t)GetCurrentThreadId(); }
#else
#include <pthread.h>
uint64_t thread_current_id(void) { return (uint64_t)(uintptr_t)pthread_self(); }
#endif
