/**
 * @file lifecycle.c
 * @ingroup core
 * @brief Lock-free module lifecycle state machine implementation
 *
 * Implements concurrent init/shutdown synchronization using atomic compare-and-swap.
 * See include/libplatform/lifecycle.h for API documentation and usage patterns.
 */

#include <libplatform/lifecycle.h>
#include <libplatform/mutex.h>
#include <libplatform/cond.h>
#define lifecycle_log_dev(...) ((void)0)
#define log_dev(...) ((void)0)

bool lifecycle_init(lifecycle_t *lc, const char *name) {
  if (lc == NULL) {
    lifecycle_log_dev("NULL lifecycle pointer");
    return false;
  }
  uint64_t expected = LIFECYCLE_UNINITIALIZED;
  if (!atomic_compare_exchange_strong(&lc->state, &expected, LIFECYCLE_INITIALIZED)) {
    lifecycle_log_dev("%s already initialized (current state: %llu)", name ? name : "<unnamed>", expected);
    return false; // Already initialized or in INITIALIZING/DEAD state
  }

  if (lc->sync_type == LIFECYCLE_SYNC_MUTEX && lc->sync.mutex) mutex_init(lc->sync.mutex, name);
  else if (lc->sync_type == LIFECYCLE_SYNC_RWLOCK && lc->sync.rwlock) rwlock_init(lc->sync.rwlock, name);
  else if (lc->sync_type == LIFECYCLE_SYNC_COND && lc->sync.cond) cond_init(lc->sync.cond, name);

  return true;
}

bool lifecycle_init_once(lifecycle_t *lc) {
  if (lc == NULL) {
    lifecycle_log_dev("NULL lifecycle pointer");
    return false;
  }

  uint64_t expected = LIFECYCLE_UNINITIALIZED;
  if (!atomic_compare_exchange_strong(&lc->state, &expected, LIFECYCLE_INITIALIZING)) {
    // If already initialized, just return false (no work needed)
    if (expected == LIFECYCLE_INITIALIZED) {
      lifecycle_log_dev("already initialized");
      return false;
    }

    // If dead, never allow re-init
    if (expected == LIFECYCLE_DEAD) {
      lifecycle_log_dev("module is dead, no re-init allowed");
      return false;
    }

    // If initializing, don't spin (caller may be retrying or it may complete asynchronously)
    // Just return false to indicate this thread doesn't need to do init work
    if (expected == LIFECYCLE_INITIALIZING) {
      lifecycle_log_dev("already initializing, skipping (will resolve asynchronously)");
      return false;
    }

    // Unexpected state
    lifecycle_log_dev("unexpected state: %d", expected);
    return false;
  }

  // Winner: state is now LIFECYCLE_INITIALIZING
  // Caller must call lifecycle_init_commit() or lifecycle_init_abort()
  lifecycle_log_dev("won CAS, transitioned to INITIALIZING");
  return true;
}

void lifecycle_init_commit(lifecycle_t *lc) {
  if (lc == NULL) {
    lifecycle_log_dev("NULL lifecycle pointer");
    return;
  }
  lifecycle_log_dev("transitioning INITIALIZING → INITIALIZED");
  atomic_store(&lc->state, LIFECYCLE_INITIALIZED);
}

void lifecycle_init_abort(lifecycle_t *lc) {
  if (lc == NULL) {
    lifecycle_log_dev("NULL lifecycle pointer");
    return;
  }
  lifecycle_log_dev("transitioning INITIALIZING → UNINITIALIZED (retry allowed)");
  atomic_store(&lc->state, LIFECYCLE_UNINITIALIZED);
}

bool lifecycle_shutdown(lifecycle_t *lc) {
  if (lc == NULL) {
    lifecycle_log_dev("NULL lifecycle pointer");
    return false;
  }
  uint64_t expected = LIFECYCLE_INITIALIZED;
  if (!atomic_compare_exchange_strong(&lc->state, &expected, LIFECYCLE_UNINITIALIZED)) {
    lifecycle_log_dev("not in INITIALIZED state (current: %d)", expected);
    return false; // Not initialized or in unexpected state
  }

  if (lc->sync_type == LIFECYCLE_SYNC_MUTEX && lc->sync.mutex) mutex_destroy(lc->sync.mutex);
  else if (lc->sync_type == LIFECYCLE_SYNC_RWLOCK && lc->sync.rwlock) rwlock_destroy(lc->sync.rwlock);
  else if (lc->sync_type == LIFECYCLE_SYNC_COND && lc->sync.cond) cond_destroy(lc->sync.cond);

  return true;
}

bool lifecycle_shutdown_forever(lifecycle_t *lc) {
  if (lc == NULL) {
    log_dev("NULL lifecycle pointer");
    return false;
  }

  uint64_t current;
  do {
    current = atomic_load(&lc->state);
    if (current == LIFECYCLE_DEAD) {
      log_dev("already DEAD");
      return false;
    }
    if (current == LIFECYCLE_INITIALIZING) {
      log_dev("spinning on INITIALIZING");
    }
  } while (current == LIFECYCLE_INITIALIZING || !atomic_compare_exchange_strong(&lc->state, &current, LIFECYCLE_DEAD));

  log_dev("transitioned to DEAD (was in state: %d)", current);
  return current == LIFECYCLE_INITIALIZED;
}

bool lifecycle_is_initialized(const lifecycle_t *lc) {
  if (lc == NULL)
    return false;
  return atomic_load(&lc->state) == LIFECYCLE_INITIALIZED;
}

bool lifecycle_is_dead(const lifecycle_t *lc) {
  if (lc == NULL)
    return false;
  return atomic_load(&lc->state) == LIFECYCLE_DEAD;
}

bool lifecycle_reset(lifecycle_t *lc) {
  if (lc == NULL) {
    log_dev("NULL lifecycle pointer");
    return false;
  }

  uint64_t expected = LIFECYCLE_INITIALIZED;
  if (!atomic_compare_exchange_strong(&lc->state, &expected, LIFECYCLE_UNINITIALIZED)) {
    log_dev("not in INITIALIZED state (current: %d)", expected);
    return false; // Not in INITIALIZED state
  }

  return true;
}

bool lifecycle_destroy_once(lifecycle_t *lc) {
  if (lc == NULL) {
    log_dev("NULL lifecycle pointer");
    return false;
  }

  uint64_t expected = LIFECYCLE_INITIALIZED;
  if (!atomic_compare_exchange_strong(&lc->state, &expected, LIFECYCLE_DESTROYING)) {
    // If not initialized, nothing to destroy
    if (expected == LIFECYCLE_UNINITIALIZED) {
      log_dev("already uninitialized, nothing to destroy");
      return false;
    }

    // If dead, never allow destruction (already permanently shut down)
    if (expected == LIFECYCLE_DEAD) {
      log_dev("module is dead, no destruction allowed");
      return false;
    }

    // If already destroying, don't duplicate work (return false, let first destroyer finish)
    if (expected == LIFECYCLE_DESTROYING) {
      log_dev("already destroying, skipping (first destroyer has priority)");
      return false;
    }

    // If initializing, skip destruction (init may still be in progress)
    if (expected == LIFECYCLE_INITIALIZING) {
      log_dev("still initializing, skipping destruction");
      return false;
    }

    // Unexpected state
    log_dev("unexpected state: %d", expected);
    return false;
  }

  // Winner: state is now LIFECYCLE_DESTROYING
  // Caller must call lifecycle_destroy_commit()
  log_dev("won CAS, transitioned to DESTROYING");
  return true;
}

void lifecycle_destroy_commit(lifecycle_t *lc) {
  if (lc == NULL) {
    log_dev("NULL lifecycle pointer");
    return;
  }
  log_dev("transitioning DESTROYING → UNINITIALIZED");
  atomic_store(&lc->state, LIFECYCLE_UNINITIALIZED);
}
