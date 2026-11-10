#include <libplatform/time.h>
#include <stdio.h>
#ifdef _WIN32
#include <windows.h>
uint64_t time_get_ns(void) { static LARGE_INTEGER f; static INIT_ONCE o=INIT_ONCE_STATIC_INIT; BOOL p=FALSE; InitOnceBeginInitialize(&o,0,&p,NULL); if(p){QueryPerformanceFrequency(&f); InitOnceComplete(&o,0,NULL);} LARGE_INTEGER c; QueryPerformanceCounter(&c); return (uint64_t)((c.QuadPart*1000000000ULL)/f.QuadPart); }
uint64_t time_get_realtime_ns(void) { FILETIME ft; GetSystemTimeAsFileTime(&ft); ULARGE_INTEGER v={.LowPart=ft.dwLowDateTime,.HighPart=ft.dwHighDateTime}; return (uint64_t)(v.QuadPart-116444736000000000ULL)*100ULL; }
#else
uint64_t time_get_ns(void) { struct timespec v; clock_gettime(CLOCK_MONOTONIC,&v); return (uint64_t)v.tv_sec*1000000000ULL+(uint64_t)v.tv_nsec; }
uint64_t time_get_realtime_ns(void) { struct timespec v; clock_gettime(CLOCK_REALTIME,&v); return (uint64_t)v.tv_sec*1000000000ULL+(uint64_t)v.tv_nsec; }
#endif
void time_ns_to_timespec(uint64_t ns, struct timespec *out) { out->tv_sec=(time_t)(ns/1000000000ULL); out->tv_nsec=(long)(ns%1000000000ULL); }
int time_pretty(uint64_t ns, int precision, char *out, size_t cap) {
  (void)precision;
  if (!cap) return 0;
  return snprintf(out, cap, "%lluns", (unsigned long long)ns);
}
