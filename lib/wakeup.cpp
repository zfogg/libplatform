#include <atomic>
#include <chrono>
#include <new>
#include <semaphore>
#include <libplatform/wakeup.h>

/**
 * @brief platform wakeup keeps the values that move through the subsystem: the fields shown below. Some members are pointers into caller- or subsystem-owned memory, so lifetime follows the API that populated them rather than the struct copy itself.
 */
struct platform_wakeup {
  std::binary_semaphore ready{0};
  std::atomic<bool> pending{false};
};

int platform_wakeup_create(platform_wakeup_t **out) {
  if (!out)
    return -1;
  auto *storage = static_cast<platform_wakeup_t *>(::operator new(sizeof(platform_wakeup_t), std::nothrow));
  if (!storage)
    return -1;
  *out = new (storage) platform_wakeup_t;
  return 0;
}

void platform_wakeup_signal(platform_wakeup_t *wake) {
  if (!wake->pending.exchange(true))
    wake->ready.release();
}

bool platform_wakeup_wait(platform_wakeup_t *wake, uint64_t timeout_ns) {
  if (!wake->ready.try_acquire_for(std::chrono::nanoseconds(timeout_ns)))
    return false;
  // The consumer inspects published work after clearing pending. A producer
  // racing this store either has its work consumed or leaves another token.
  wake->pending.store(false);
  return true;
}

void platform_wakeup_destroy(platform_wakeup_t *wake) {
  if (wake) {
    wake->~platform_wakeup();
    ::operator delete(wake);
  }
}
