#pragma once

/* Compiler and ABI portability declarations shared by libplatform clients. */
#if defined(_WIN32)
#define LIBPLATFORM_WINDOWS 1
#define LIBPLATFORM_POSIX 0
#define PLATFORM_WINDOWS 1
#define PLATFORM_POSIX 0
#define PACKED_STRUCT_BEGIN __pragma(pack(push, 1))
#define PACKED_STRUCT_END __pragma(pack(pop))
#define PACKED_ATTR
#define ALIGNED_ATTR(value) __declspec(align(value))
#if defined(_MSC_VER)
#define THREAD_LOCAL __declspec(thread)
#else
#define THREAD_LOCAL __thread
#endif
#ifndef PATH_MAX
#define PATH_MAX 260
#endif
#else
#define LIBPLATFORM_WINDOWS 0
#define LIBPLATFORM_POSIX 1
#define PLATFORM_WINDOWS 0
#define PLATFORM_POSIX 1
#define PACKED_STRUCT_BEGIN
#define PACKED_STRUCT_END
#define PACKED_ATTR __attribute__((packed))
#define ALIGNED_ATTR(value) __attribute__((aligned(value)))
#ifndef THREAD_LOCAL
#define THREAD_LOCAL __thread
#endif
#include <limits.h>
#endif

#ifndef UNUSED
#define UNUSED(value) ((void)(value))
#endif
