/**
 * @file platform/windows/agent.c
 * @brief Windows SSH/GPG agent socket discovery implementation
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

int platform_get_ssh_agent_socket(char *path_out, size_t path_size) {
  /* Windows: Check SSH_AUTH_SOCK first */
  const char *auth_sock = platform_getenv("SSH_AUTH_SOCK");
  if (auth_sock && strlen(auth_sock) > 0) {
    return copy_agent_path(auth_sock, path_out, path_size);
  }

  /* Windows default: named pipe for openssh-ssh-agent */
  const char *default_pipe = "\\\\.\\pipe\\openssh-ssh-agent";
  return copy_agent_path(default_pipe, path_out, path_size);
}

int platform_get_gpg_agent_socket(char *path_out, size_t path_size) {
  /* Windows: Try gpgconf first */
  FILE *fp = NULL;
  if (platform_popen("gpgconf", "gpgconf --list-dirs agent-socket 2>nul", "r", &fp) == 0 && fp) {
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

  /* Fallback to default GPG4Win location */
  const char *appdata = platform_getenv("APPDATA");
  if (appdata) {
    size_t needed = strlen(appdata) + strlen("\\gnupg\\S.gpg-agent") + 1;
    if (needed > path_size) {
      return -1;
    }
    snprintf(path_out, path_size, "%s\\gnupg\\S.gpg-agent", appdata);
    return 0;
  }

  return -1;
}
