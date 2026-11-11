#pragma once

#include <stddef.h>
#ifdef _WIN32
#include <winsock2.h>
#else
#include <sys/socket.h>
#endif

int format_ip_address(int family, const struct sockaddr *address, char *output, size_t output_size);
