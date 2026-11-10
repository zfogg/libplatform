#include <libplatform/util/ip.h>
#ifdef _WIN32
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#endif

int format_ip_address(int family, const struct sockaddr *address, char *output, size_t output_size) {
  if (address == NULL || output == NULL || output_size == 0)
    return -1;
  const void *source = family == AF_INET ? (const void *)&((const struct sockaddr_in *)address)->sin_addr
                                         : family == AF_INET6 ? (const void *)&((const struct sockaddr_in6 *)address)->sin6_addr
                                                              : NULL;
  return source != NULL && inet_ntop(family, source, output, (socklen_t)output_size) != NULL ? 0 : -1;
}
