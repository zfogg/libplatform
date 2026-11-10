/** @file system_common.c @brief Common descriptor and formatting helpers. */

#include <libplatform/system.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

int platform_dup(int fd) {
#ifdef _WIN32
  return _dup(fd);
#else
  return dup(fd);
#endif
}

int platform_fileno(FILE *stream) {
#ifdef _WIN32
  return _fileno(stream);
#else
  return fileno(stream);
#endif
}

int platform_dup2(int source_fd, int target_fd) {
#ifdef _WIN32
  return _dup2(source_fd, target_fd);
#else
  return dup2(source_fd, target_fd);
#endif
}

int safe_vsnprintf(char *buffer, size_t buffer_size, const char *format, va_list ap) {
  if (!format || (!buffer && buffer_size != 0))
    return -1;
  return vsnprintf(buffer, buffer_size, format, ap);
}

int safe_snprintf(char *buffer, size_t buffer_size, const char *format, ...) {
  va_list ap;
  va_start(ap, format);
  int result = safe_vsnprintf(buffer, buffer_size, format, ap);
  va_end(ap);
  return result;
}

int safe_fprintf(FILE *stream, const char *format, ...) {
  if (!stream || !format)
    return -1;
  va_list ap;
  va_start(ap, format);
  int result = vfprintf(stream, format, ap);
  va_end(ap);
  return result;
}

size_t platform_write_all(int fd, const void *buffer, size_t count) {
  if (!buffer || count == 0)
    return 0;
  const unsigned char *bytes = buffer;
  size_t written = 0;
  while (written < count) {
#ifdef _WIN32
    int result = _write(fd, bytes + written, (unsigned int)(count - written));
#else
    ssize_t result = write(fd, bytes + written, count - written);
#endif
    if (result > 0) {
      written += (size_t)result;
      continue;
    }
    if (result < 0 && errno == EINTR)
      continue;
    break;
  }
  return written;
}
