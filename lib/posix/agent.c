/**
 * @file platform/posix/agent.c
 * @brief POSIX SSH/GPG agent socket discovery implementation
 * @ingroup platform
 */

#include <libplatform/agent.h>
#include <libplatform/process.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static int copy_agent_path(const char *path, char *path_out, size_t path_size) {
  if (!path || path[0] == '\0' || !path_out || path_size == 0 || strlen(path) >= path_size) return -1;
  memcpy(path_out, path, strlen(path) + 1);
  return 0;
}
#include <unistd.h>

int platform_get_ssh_agent_socket(char *path_out, size_t path_size) {
  /* Unix: SSH_AUTH_SOCK is required */
  const char *auth_sock = platform_getenv("SSH_AUTH_SOCK");
  return copy_agent_path(auth_sock, path_out, path_size);
}

int platform_get_gpg_agent_socket(char *path_out, size_t path_size) {
  /* Try gpgconf first */
  FILE *fp = NULL;
  if (platform_popen("gpgconf", "gpgconf --list-dirs agent-socket 2>/dev/null", "r", &fp) == 0 && fp) {
    if (fgets(path_out, path_size, fp)) {
      /* Remove trailing newline */
      size_t len = strlen(path_out);
      if (len > 0 && path_out[len - 1] == '\n') {
        path_out[len - 1] = '\0';
      }
      platform_pclose(&fp);
      return 0;
    }
    platform_pclose(&fp);
  }

  /* Fallback: ~/.gnupg/S.gpg-agent */
  const char *home = platform_getenv("HOME");
  if (home && home[0] != '\0') {
    size_t needed = strlen(home) + strlen("/.gnupg/S.gpg-agent") + 1;
    if (needed > path_size) {
      return -1;
    }
    snprintf(path_out, path_size, "%s/.gnupg/S.gpg-agent", home);
    return 0;
  }

  return -1;
}
