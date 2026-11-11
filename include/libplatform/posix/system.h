#pragma once

/**
 * @file posix/system.h
 * @brief POSIX system compatibility definitions
 *
 * This header collects the small POSIX constants and shell conventions used
 * by libplatform's common system and process implementations. It is shared by
 * Linux and Apple targets and contains no application policy.
 */

#include <unistd.h>
#include <limits.h>

#ifndef STDIN_FILENO
/** Standard input, output, and error file descriptors. */
#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2
#endif

#ifndef PLATFORM_SHELL_NULL_REDIRECT
/** POSIX shell fragment that discards standard error output. */
#define PLATFORM_SHELL_NULL_REDIRECT "2>/dev/null"
#endif
