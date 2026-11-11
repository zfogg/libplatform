/*
 * libwebsockets references this callback when its optional service-loop
 * sampling support is enabled.  iOS does not expose the process-spawn
 * sampling hooks used by desktop builds, so keep the optional callback a
 * no-op in the platform library.
 */
#include <stddef.h>

void lws_spawn_tree_sample_cb(void *sul) {
  (void)sul;
}
