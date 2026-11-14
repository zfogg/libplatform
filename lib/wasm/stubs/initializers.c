/**
 * @file platform/wasm/stubs/initializers.c
 * @brief Initialization stubs for platform services unavailable in WASM.
 * @ingroup platform
 */

int keyboard_init(void) {
  return 0;
}

int socket_init(void) {
  return 0;
}
