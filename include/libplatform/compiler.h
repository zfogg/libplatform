#pragma once

/* Compiler and ABI portability declarations shared by libplatform clients. */
#include <limits.h>
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
#define PLATFORM_MAX_PATH_LENGTH 32767
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
#if defined(__APPLE__)
#include <sys/syslimits.h>
#if defined(__has_include)
#if __has_include(<TargetConditionals.h>)
#include <TargetConditionals.h>
#endif
#endif
#define PLATFORM_DARWIN 1
#define PLATFORM_LINUX 0
#if defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE
#ifndef PLATFORM_IOS
#define PLATFORM_IOS 1
#endif
#else
#ifndef PLATFORM_IOS
#define PLATFORM_IOS 0
#endif
#endif
#elif defined(__linux__)
#define PLATFORM_DARWIN 0
#define PLATFORM_LINUX 1
#define PLATFORM_IOS 0
#else
#define PLATFORM_DARWIN 0
#define PLATFORM_LINUX 0
#define PLATFORM_IOS 0
#endif
#if defined(PATH_MAX)
#define PLATFORM_MAX_PATH_LENGTH PATH_MAX
#else
#define PLATFORM_MAX_PATH_LENGTH 4096
#endif
#endif

#ifndef UNUSED
#define UNUSED(value) ((void)(value))
#endif
