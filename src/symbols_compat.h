#pragma once
#include <libplatform/symbols.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#if !defined(_WIN32)
#include <unistd.h>
#else
#include <windows.h>
#define strdup _strdup
#endif

#define ASCIICHAT_OK LIBPLATFORM_OK
#define asciichat_error_t libplatform_status_t
#define PLATFORM_MAX_PATH_LENGTH 4096
#define PLATFORM_BACKTRACE_FRAME_CAPACITY 64
#define NULL_SENTINEL "[NULL]"
#define SAFE_MALLOC(n, type) ((type)malloc((n)))
#define SAFE_CALLOC(n, s, type) ((type)calloc((n), (s)))
#define SAFE_FREE(p) free((p))
#define SAFE_SNPRINTF snprintf
#define SAFE_STRNCPY(d, s, n) do { strncpy((d), (s), (n)); (d)[(n) - 1] = '\0'; } while (0)
#define SAFE_GETENV getenv
#define platform_strdup strdup
#define safe_snprintf snprintf
#if defined(_WIN32)
#define popen _popen
#define pclose _pclose
#endif
#define ERROR_INVALID_PARAM 1
#define ERROR_THREAD 2
#define ERROR_BUFFER_OVERFLOW 3
#define SET_ERRNO(code, ...) (code)

typedef struct { atomic_uint_fast64_t value; } atomic_t;
static inline bool atomic_load_bool(const atomic_t *v) { return atomic_load(&v->value) != 0; }
static inline void atomic_store_bool(atomic_t *v, bool x) { atomic_store(&v->value, x ? 1 : 0); }
static inline bool atomic_cas_bool(atomic_t *v, bool *expected, bool desired) { uint64_t e = *expected ? 1 : 0; bool ok = atomic_compare_exchange_strong(&v->value, &e, desired ? 1 : 0); if (!ok) *expected = e != 0; return ok; }
static inline void atomic_fetch_add_u64(atomic_t *v, uint64_t x) { (void)atomic_fetch_add(&v->value, x); }
static inline void atomic_store_u64(atomic_t *v, uint64_t x) { atomic_store(&v->value, x); }
static inline uint64_t atomic_load_u64(const atomic_t *v) { return atomic_load(&v->value); }

typedef struct { atomic_bool initialized; } lifecycle_t;
#define LIFECYCLE_INIT { 0 }
static inline bool lifecycle_init(lifecycle_t *l, const char *name) { (void)name; bool e = false; return atomic_compare_exchange_strong(&l->initialized, &e, true) || e; }
static inline void lifecycle_init_abort(lifecycle_t *l) { atomic_store(&l->initialized, false); }
static inline bool lifecycle_is_initialized(const lifecycle_t *l) { return atomic_load(&l->initialized); }
static inline bool lifecycle_shutdown(lifecycle_t *l) { bool e = true; return atomic_compare_exchange_strong(&l->initialized, &e, false); }

typedef struct { atomic_flag lock; } rwlock_t;
#define RWLOCK_INIT { ATOMIC_FLAG_INIT }
static inline int rwlock_init(rwlock_t *l, const char *name) { (void)name; atomic_flag_clear(&l->lock); return 0; }
static inline void rwlock_destroy(rwlock_t *l) { (void)l; }
static inline void rwlock_rdlock(rwlock_t *l) { while (atomic_flag_test_and_set(&l->lock)) {} }
static inline void rwlock_rdunlock(rwlock_t *l) { atomic_flag_clear(&l->lock); }
static inline void rwlock_wrlock(rwlock_t *l) { rwlock_rdlock(l); }
static inline void rwlock_wrunlock(rwlock_t *l) { rwlock_rdunlock(l); }

typedef struct { char path[PLATFORM_MAX_PATH_LENGTH]; uintptr_t symbolizer_address; bool relative_address; } platform_binary_match_t;
static inline int platform_get_executable_path(char *out, size_t size) {
#if defined(_WIN32)
  return GetModuleFileNameA(NULL, out, (unsigned long)size) != 0;
#else
  ssize_t n = readlink("/proc/self/exe", out, size - 1); if (n < 0) return 0; out[n] = 0; return 1;
#endif
}
static inline bool platform_is_binary_in_path(const char *name) { (void)name; return true; }
static inline int platform_popen(const char *name, const char *command, const char *mode, FILE **out) { (void)name; *out = popen(command, mode); return *out ? 0 : -1; }
static inline void platform_pclose(FILE **f) { if (f && *f) { pclose(*f); *f = NULL; } }
static inline bool validate_shell_safe(const char *s, const char *allowed) { (void)allowed; return s && !strpbrk(s, ";&|`\n\r"); }
static inline bool escape_path_for_shell(const char *in, char *out, size_t n) { return in && snprintf(out, n, "\"%s\"", in) > 0; }
static inline const char *extract_project_relative_path(const char *p) { const char *a = strstr(p, "ascii-chat/"); return a ? a + 11 : p; }
static inline const char *path_last_components(const char *p, int count) { const char *q = p + strlen(p); while (count-- > 0) { while (q > p && q[-1] != '/' && q[-1] != '\\') --q; if (q > p) --q; } return q; }
static inline bool terminal_supports_utf8(void) { return true; }
static inline int get_binary_file_address_offsets(void *addr, platform_binary_match_t *out, int max) { (void)addr; (void)max; if (!out || !platform_get_executable_path(out->path, sizeof(out->path))) return 0; out->symbolizer_address = (uintptr_t)addr; out->relative_address = false; return 1; }

typedef struct { void *next; } UT_hash_handle;
#define HASH_FIND_PTR(head, keyptr, out) do { (out) = NULL; for (void *_p = (head); _p; _p = ((typeof(out))_p)->hh.next) if (((typeof(out))_p)->addr == *(keyptr)) { (out) = (typeof(out))_p; break; } } while (0)
#define HASH_ADD_PTR(head, field, item) do { (item)->hh.next = (head); (head) = (item); } while (0)
#define HASH_DEL(head, item) do { void **_p = (void **)&(head); while (*_p && *(_p) != (void *)(item)) _p = &((typeof(item))*_p)->hh.next; if (*_p) *_p = (item)->hh.next; } while (0)
#define HASH_ITER(hh, head, item, tmp) for ((item) = (head); (item) && ((tmp) = (typeof(item))(item)->hh.next, 1); (item) = (tmp))
#define HASH_COUNT(head) ({ size_t _n = 0; for (void *_p = (head); _p; _p = ((typeof(head))_p)->hh.next) ++_n; _n; })
