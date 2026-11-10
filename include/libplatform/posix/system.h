#pragma once

#include <unistd.h>
#include <limits.h>

#ifndef STDIN_FILENO
#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2
#endif

#ifndef PLATFORM_SHELL_NULL_REDIRECT
#define PLATFORM_SHELL_NULL_REDIRECT "2>/dev/null"
#endif
