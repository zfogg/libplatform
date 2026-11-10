/** Portable vectored sends and owned Linux copy-avoidance storage. */

/* Storage capacities in elements; text buffers include the terminating NUL. */
#define PLATFORM_SOCKET_IO_BYTES_BUFFER_SIZE 512

#include <libplatform/socket.h>
#include <stdlib.h>
#include <libplatform/time.h>
#include <limits.h>
#include <string.h>
#ifndef _WIN32
#include <sys/uio.h>
#include <sys/time.h>
#include <errno.h>
#endif
#ifdef __linux__
#include <sys/mman.h>
#include <unistd.h>
#include <linux/errqueue.h>
#endif

static _Thread_local socket_io_stats_t io_stats;
socket_io_stats_t socket_io_stats_get(void) {
  return io_stats;
}
void socket_io_stats_reset(void) {
  io_stats = (socket_io_stats_t){0};
}

static int validate_vectors(const socket_buffer_t *buffers, size_t count, size_t *total) {
  if (!buffers || !count || count > SOCKET_IOV_MAX)
    return -1;
  *total = 0;
  for (size_t i = 0; i < count; ++i) {
    if ((buffers[i].len && !buffers[i].data) || buffers[i].len > (size_t)INT_MAX - *total)
      return -1;
    *total += buffers[i].len;
  }
  return 0;
}

int socket_sendv(socket_t sock, const socket_buffer_t *buffers, size_t count, size_t *sent) {
  size_t total;
  if (!sent)
    return -1;
  *sent = 0;
  int err = validate_vectors(buffers, count, &total);
  if (err != 0 || !total)
    return err;
  ++io_stats.send_calls;
#ifdef _WIN32
  WSABUF vectors[SOCKET_IOV_MAX];
  for (size_t i = 0; i < count; ++i) {
    vectors[i].buf = (char *)buffers[i].data;
    vectors[i].len = (ULONG)buffers[i].len;
  }
  DWORD bytes = 0;
  if (WSASend(sock, vectors, (DWORD)count, &bytes, 0, NULL, NULL) == SOCKET_ERROR) {
    int error = WSAGetLastError();
    if (error == WSAEWOULDBLOCK || error == WSAEINTR)
      return 0;
    return -1;
  }
  *sent = bytes;
#else
  struct iovec vectors[SOCKET_IOV_MAX];
  for (size_t i = 0; i < count; ++i) {
    vectors[i].iov_base = (void *)buffers[i].data;
    vectors[i].iov_len = buffers[i].len;
  }
  struct msghdr msg = {0};
  msg.msg_iov = vectors;
  msg.msg_iovlen = count;
  int flags = MSG_DONTWAIT;
#ifdef MSG_NOSIGNAL
  flags |= MSG_NOSIGNAL;
#elif defined(SO_NOSIGPIPE)
  int one = 1;
  if (setsockopt(sock, SOL_SOCKET, SO_NOSIGPIPE, &one, sizeof(one)) != 0)
    return -1;
#endif
  ssize_t bytes = sendmsg(sock, &msg, flags);
  if (bytes < 0) {
    if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
      return 0;
    return -1;
  }
  *sent = (size_t)bytes;
#endif
  io_stats.sent_bytes += *sent;
  if (!*sent)
    return -1;
  return 0;
}

int socket_sendv_all(socket_t sock, const socket_buffer_t *buffers, size_t count, uint64_t timeout_ns) {
  size_t total;
  int result = validate_vectors(buffers, count, &total);
  if (result != 0 || !total)
    return result;
  if (!timeout_ns || timeout_ns > INT64_MAX)
    return -1;
  socket_buffer_t pending[SOCKET_IOV_MAX];
  memcpy(pending, buffers, count * sizeof(*buffers));
  size_t index = 0, sent_total = 0;
  uint64_t start = time_get_ns();
#if defined(_WIN32) || defined(__APPLE__)
#ifdef _WIN32
  DWORD original_timeout = 0;
#else
  struct timeval original_timeout = {0};
#endif
  socklen_t option_len = sizeof(original_timeout);
  if (socket_getsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &original_timeout, &option_len) != 0)
    return -1;
#endif
  while (sent_total < total) {
    uint64_t elapsed = time_get_ns() - start;
    if (elapsed >= timeout_ns) {
      result = -1;
      break;
    }
    struct pollfd ready = {.fd = sock, .events = POLLOUT};
    int polled = socket_poll(&ready, 1, (int64_t)(timeout_ns - elapsed));
    if (polled < 0) {
#ifdef _WIN32
      if (socket_get_last_error() == WSAEINTR)
#else
      if (socket_get_last_error() == EINTR)
#endif
        continue;
    }
    if (polled <= 0 || !(ready.revents & POLLOUT)) {
      result = -1;
      break;
    }
#if defined(_WIN32) || defined(__APPLE__)
    // Winsock has no MSG_DONTWAIT, and Darwin can stall a large vectored write
    // against a small socket buffer. Bound the syscall as well as its readiness
    // wait, preserving the caller's blocking mode and configured timeout.
    elapsed = time_get_ns() - start;
    if (elapsed >= timeout_ns) {
      result = -1;
      break;
    }
    uint64_t remaining_ms = (timeout_ns - elapsed + 999999) / 1000000;
#ifdef _WIN32
    DWORD timeout = (DWORD)(remaining_ms > INT_MAX ? INT_MAX : remaining_ms);
#else
    struct timeval timeout = {.tv_sec = (time_t)(remaining_ms / 1000),
                              .tv_usec = (suseconds_t)((remaining_ms % 1000) * 1000)};
#endif
    if (socket_setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) != 0) {
      result = -1;
      break;
    }
#endif
    while (index < count && !pending[index].len)
      ++index;
    size_t sent = 0;
    result = socket_sendv(sock, pending + index, count - index, &sent);
    if (result != 0) {
      // A failing blocking WSASend may have consumed bytes without reporting a
      // count. Conservatively terminate the stream rather than retry a packet.
      socket_shutdown(sock, SHUT_RDWR);
      break;
    }
    sent_total += sent;
    while (sent && index < count) {
      if (!pending[index].len) {
        ++index;
        continue;
      }
      size_t advance = sent < pending[index].len ? sent : pending[index].len;
      pending[index].data = (const char *)pending[index].data + advance;
      pending[index].len -= advance;
      sent -= advance;
      if (!pending[index].len)
        ++index;
    }
  }
#if defined(_WIN32) || defined(__APPLE__)
  if (socket_setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &original_timeout, sizeof(original_timeout)) != 0)
    result = -1;
#endif
  if (result != 0 && sent_total) {
    socket_shutdown(sock, SHUT_RDWR);
    return -1;
  }
  return result;
}

int socket_send_buffer_alloc(size_t capacity, socket_send_buffer_t *buffer) {
  if (!buffer || !capacity || capacity > SOCKET_SEND_BUFFER_MAX)
    return -1;
  *buffer = (socket_send_buffer_t){0};
#ifdef __linux__
  if (capacity >= SOCKET_ZEROCOPY_THRESHOLD) {
    long page = sysconf(_SC_PAGESIZE);
    if (page <= 0 || capacity > SIZE_MAX - (size_t)page + 1)
      return -1;
    void *data = mmap(NULL, capacity, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (data != MAP_FAILED) {
      buffer->data = data;
      buffer->capacity = capacity;
      buffer->mapped = true;
      return 0;
    }
    
  }
#endif
  buffer->data = malloc(capacity);
  if (!buffer->data)
    return -1;
  buffer->capacity = capacity;
  return 0;
}

void socket_send_buffer_free(socket_send_buffer_t *buffer) {
  if (!buffer || !buffer->data)
    return;
#ifdef __linux__
  if (buffer->mapped)
    munmap(buffer->data, buffer->capacity);
  else
#endif
    free(buffer->data);
  
  *buffer = (socket_send_buffer_t){0};
}

int socket_zerocopy_try(socket_t sock, const socket_send_buffer_t *buffer, size_t offset, size_t length,
                                      size_t *sent, bool *supported) {
  if (!buffer || !buffer->data || !sent || !supported || offset > buffer->capacity ||
      length > buffer->capacity - offset)
    return -1;
  *sent = 0;
  *supported = false;
#if defined(__linux__) && defined(SO_ZEROCOPY) && defined(MSG_ZEROCOPY)
  if (!buffer->mapped || length < SOCKET_ZEROCOPY_THRESHOLD)
    return 0;
  int one = 1;
  if (setsockopt(sock, SOL_SOCKET, SO_ZEROCOPY, &one, sizeof(one)) != 0)
    return 0;
  *supported = true;
  ++io_stats.send_calls;
  ssize_t count = send(sock, (const char *)buffer->data + offset, length, MSG_ZEROCOPY | MSG_DONTWAIT | MSG_NOSIGNAL);
  if (count < 0) {
    if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
      return 0;
    if (errno == ENOBUFS || errno == EOPNOTSUPP || errno == EINVAL) {
      *supported = false;
      return 0;
    }
    return -1;
  }
  if (!count)
    return -1;
  *sent = (size_t)count;
  io_stats.sent_bytes += *sent;
  ++io_stats.zerocopy_calls;
#else
  (void)sock;
#endif
  return 0;
}

int socket_zerocopy_poll(socket_t sock, bool *completed, bool *copied) {
  if (!completed || !copied)
    return -1;
  *completed = *copied = false;
#if defined(__linux__) && defined(SO_ZEROCOPY) && defined(MSG_ZEROCOPY)
  union {
    struct cmsghdr align;
    char bytes[PLATFORM_SOCKET_IO_BYTES_BUFFER_SIZE];
  } control;
  struct msghdr message = {0};
  message.msg_control = control.bytes;
  message.msg_controllen = sizeof(control.bytes);
  if (recvmsg(sock, &message, MSG_ERRQUEUE | MSG_DONTWAIT) < 0) {
    if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
      return 0;
    return -1;
  }
  if (message.msg_flags & MSG_CTRUNC)
    return -1;
  for (struct cmsghdr *c = CMSG_FIRSTHDR(&message); c; c = CMSG_NXTHDR(&message, c)) {
    if (!((c->cmsg_level == SOL_IP && c->cmsg_type == IP_RECVERR) ||
          (c->cmsg_level == SOL_IPV6 && c->cmsg_type == IPV6_RECVERR)) ||
        c->cmsg_len < CMSG_LEN(sizeof(struct sock_extended_err)))
      continue;
    struct sock_extended_err error;
    memcpy(&error, CMSG_DATA(c), sizeof(error));
    if (error.ee_errno || error.ee_origin != SO_EE_ORIGIN_ZEROCOPY || error.ee_info != error.ee_data)
      return -1;
    *completed = true;
    *copied = (error.ee_code & SO_EE_CODE_ZEROCOPY_COPIED) != 0;
    ++io_stats.zerocopy_completions;
    if (*copied)
      ++io_stats.zerocopy_copied;
  }
#else
  (void)sock;
#endif
  return 0;
}

int socket_send_zerocopy(socket_t sock, const socket_send_buffer_t *buffer, size_t len,
                                       uint64_t timeout_ns, bool *supported, bool *copied) {
  if (!supported || !copied || !buffer || !buffer->data || len > buffer->capacity || !timeout_ns ||
      timeout_ns > INT64_MAX)
    return -1;
  *supported = false;
  *copied = false;
#if defined(__linux__) && defined(SO_ZEROCOPY) && defined(MSG_ZEROCOPY)
  if (!buffer->mapped || len < SOCKET_ZEROCOPY_THRESHOLD)
    return 0;
  int one = 1;
  if (setsockopt(sock, SOL_SOCKET, SO_ZEROCOPY, &one, sizeof(one)) != 0)
    return 0;
  uint64_t start = time_get_ns();
  size_t offset = 0;
  while (offset < len) {
    uint64_t elapsed = time_get_ns() - start;
    if (elapsed >= timeout_ns)
      goto failed;
    struct pollfd ready = {.fd = sock, .events = POLLOUT};
    int polled = socket_poll(&ready, 1, (int64_t)(timeout_ns - elapsed));
    if (polled < 0 && errno == EINTR)
      continue;
    if (polled <= 0 || !(ready.revents & POLLOUT))
      goto failed;
    ++io_stats.send_calls;
    ssize_t n =
        send(sock, (const char *)buffer->data + offset, len - offset, MSG_ZEROCOPY | MSG_DONTWAIT | MSG_NOSIGNAL);
    if (n < 0) {
      if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)
        continue;
      if (errno == ENOBUFS || errno == EOPNOTSUPP || errno == EINVAL) {
        if (!offset)
          return 0;
        // Earlier submissions are completed. Fall back only for unsent bytes.
        *copied = true;
        goto remaining_copy;
      }
      goto failed;
    }
    if (!n)
      goto failed;
    *supported = true;
    offset += (size_t)n;
    io_stats.sent_bytes += (size_t)n;
    ++io_stats.zerocopy_calls;
    // One outstanding submission bounds pinned storage to one packet. Drain its
    // completion before submitting again; no cookie ordering or wrap assumption.
    bool completed = false;
    while (!completed) {
      elapsed = time_get_ns() - start;
      if (elapsed >= timeout_ns)
        goto failed;
      struct pollfd completion = {.fd = sock, .events = 0};
      polled = socket_poll(&completion, 1, (int64_t)(timeout_ns - elapsed));
      if (polled < 0 && errno == EINTR)
        continue;
      if (polled <= 0)
        goto failed;
      union {
        struct cmsghdr align;
        char bytes[PLATFORM_SOCKET_IO_BYTES_BUFFER_SIZE];
      } control;
      struct msghdr msg = {0};
      msg.msg_control = control.bytes;
      msg.msg_controllen = sizeof(control.bytes);
      if (recvmsg(sock, &msg, MSG_ERRQUEUE | MSG_DONTWAIT) < 0) {
        if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK)
          continue;
        goto failed;
      }
      if (msg.msg_flags & MSG_CTRUNC)
        goto failed;
      for (struct cmsghdr *c = CMSG_FIRSTHDR(&msg); c; c = CMSG_NXTHDR(&msg, c)) {
        if (!((c->cmsg_level == SOL_IP && c->cmsg_type == IP_RECVERR) ||
              (c->cmsg_level == SOL_IPV6 && c->cmsg_type == IPV6_RECVERR)) ||
            c->cmsg_len < CMSG_LEN(sizeof(struct sock_extended_err)))
          continue;
        struct sock_extended_err e;
        memcpy(&e, CMSG_DATA(c), sizeof(e));
        if (e.ee_errno)
          goto failed;
        if (e.ee_origin == SO_EE_ORIGIN_ZEROCOPY) {
          if (e.ee_info != e.ee_data)
            goto failed; // Another owner has used this socket's zerocopy queue.
          completed = true;
          ++io_stats.zerocopy_completions;
          if (e.ee_code & SO_EE_CODE_ZEROCOPY_COPIED)
            ++io_stats.zerocopy_copied;
          *copied |= (e.ee_code & SO_EE_CODE_ZEROCOPY_COPIED) != 0;
        }
      }
    }
    if (*copied && offset < len) {
    remaining_copy:
      elapsed = time_get_ns() - start;
      if (elapsed >= timeout_ns)
        goto failed;
      socket_buffer_t remaining = {(const char *)buffer->data + offset, len - offset};
      if (socket_sendv_all(sock, &remaining, 1, timeout_ns - elapsed) != 0)
        goto failed;
      return 0;
    }
  }
  return 0;
failed:
  // Unmapping does not recycle physical pages pinned by the kernel. The caller
  // must discard this mapping, never write into it after an unsuccessful send.
  *supported = true;
  socket_shutdown(sock, SHUT_RDWR);
  return -1;
#else
  (void)sock;
  return 0;
#endif
}
