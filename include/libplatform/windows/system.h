#pragma once

#include <fcntl.h>
#include <io.h>
#include <process.h>

#ifndef STDIN_FILENO
#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2
#endif
#ifndef PATH_MAX
#define PATH_MAX 260
#endif
#ifndef O_CLOEXEC
#define O_CLOEXEC 0
#endif
#ifndef S_IRUSR
#define S_IRUSR 0400
#define S_IWUSR 0200
#define S_IXUSR 0100
#define S_IRGRP 0040
#define S_IWGRP 0020
#define S_IXGRP 0010
#define S_IROTH 0004
#define S_IWOTH 0002
#define S_IXOTH 0001
#endif
#ifndef close
#define close _close
#define read _read
#define write _write
#define lseek _lseek
#define unlink _unlink
#define isatty _isatty
#define getpid _getpid
#endif
#ifndef CLOCK_REALTIME
#define CLOCK_REALTIME 0
#define CLOCK_MONOTONIC 1
#endif

#ifndef PLATFORM_SHELL_NULL_REDIRECT
#define PLATFORM_SHELL_NULL_REDIRECT "2>nul"
#endif
