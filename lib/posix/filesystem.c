/**
 * @file platform/posix/filesystem.c
 * @brief POSIX filesystem operations
 * @ingroup platform
 */

#ifndef _WIN32

/* Storage capacities in elements; text buffers include the terminating NUL. */
#define PLATFORM_POSIX_FILESYSTEM_TMP_BUFFER_SIZE 512
#define PLATFORM_POSIX_FILESYSTEM_BIN_WITH_SUFFIX_BUFFER_SIZE 512

#include <libplatform/filesystem.h>
#include "util/string.h"
#include <libplatform/system.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdint.h>
#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

const char *platform_getenv(const char *name) {
  return name ? getenv(name) : NULL;
}

int platform_open(const char *name, const char *pathname, int flags, ...) {
  (void)name;
  if (!pathname)
    return -1;
  mode_t mode = 0;
  if (flags & O_CREAT) {
    va_list args;
    va_start(args, flags);
    mode = (mode_t)va_arg(args, int);
    va_end(args);
  }
  return open(pathname, flags, mode);
}

// ============================================================================
// Directory Management
// ============================================================================

/**
 * @brief Create a directory (POSIX implementation)
 */
int platform_mkdir(const char *path, int mode) {
  if (!path) {
    return -1;
  }

  if (mkdir(path, mode) == -1) {
    // EEXIST is not an error - directory may already exist
    if (errno == EEXIST) {
      // Verify it's actually a directory (use stat() to follow symlinks)
      struct stat sb;
      if (stat(path, &sb) == 0 && S_ISDIR(sb.st_mode)) {
        return 0;
      }
      // Path exists but is not a directory
      return -1;
    }
    return -1;
  }

  return 0;
}

/**
 * @brief Create directories recursively (POSIX implementation)
 */
int platform_mkdir_recursive(const char *path, int mode) {
  if (!path || path[0] == '\0') {
    return -1;
  }

  char tmp[PLATFORM_POSIX_FILESYSTEM_TMP_BUFFER_SIZE];
  size_t len = strlen(path);
  if (len >= sizeof(tmp)) {
    return -1;
  }

  strncpy(tmp, path, sizeof(tmp) - 1);
  tmp[sizeof(tmp) - 1] = '\0';

  // Create each directory in the path
  for (char *p = tmp + 1; *p; p++) {
    if (*p == '/' || *p == '\\') {
      char orig = *p;
      *p = '\0';

      // Skip empty components
      if (tmp[0] != '\0' && strcmp(tmp, ".") != 0) {
        if (mkdir(tmp, mode) != 0 && errno != EEXIST) {
          // Propagate error if not EEXIST
          return -1;
        }
      }

      *p = orig;
    }
  }

  // Create the final directory
  if (mkdir(tmp, mode) != 0 && errno != EEXIST) {
    return -1;
  }

  return 0;
}

// ============================================================================
// File Statistics
// ============================================================================

/**
 * @brief Get file statistics (POSIX implementation)
 */
int platform_stat(const char *path, platform_stat_t *stat_out) {
  if (!path || !stat_out) {
    return -1;
  }

  struct stat sb;
  if (lstat(path, &sb) == -1) {
    return ENOENT;
  }

  stat_out->size = (size_t)sb.st_size;
  stat_out->mode = sb.st_mode;
  stat_out->is_regular_file = S_ISREG(sb.st_mode) ? 1 : 0;
  stat_out->is_directory = S_ISDIR(sb.st_mode) ? 1 : 0;
  stat_out->is_symlink = S_ISLNK(sb.st_mode) ? 1 : 0;

  return 0;
}

/**
 * @brief Check if a path is a regular file (POSIX implementation)
 */
int platform_is_regular_file(const char *path) {
  if (!path) {
    -1;
    return 0;
  }

  platform_stat_t stat_info;
  if (platform_stat(path, &stat_info) != 0) {
    return 0;
  }

  return stat_info.is_regular_file;
}

/**
 * @brief Check if a path is a directory (POSIX implementation)
 */
int platform_is_directory(const char *path) {
  if (!path) {
    -1;
    return 0;
  }

  platform_stat_t stat_info;
  if (platform_stat(path, &stat_info) != 0) {
    return 0;
  }

  return stat_info.is_directory;
}

// ============================================================================
// Temporary Files and Directories
// ============================================================================

int platform_create_temp_file(char *path_out, size_t path_size, const char *prefix, int *fd) {
  /* Unix temp file creation with mkstemp() - include PID for concurrent process safety */
  int needed = snprintf(path_out, path_size, "/tmp/%s_%d_XXXXXX", prefix, getpid());
  if (needed < 0 || (size_t)needed >= path_size) {
    return -1;
  }

  int temp_fd = mkstemp(path_out);
  if (temp_fd < 0) {
    return -1;
  }

  *fd = temp_fd;
  return 0;
}

int platform_delete_temp_file(const char *path) {
  return unlink(path);
}

int platform_mkdtemp(char *path_out, size_t path_size, const char *prefix) {
  if (!path_out || path_size < 32 || !prefix) {
    return -1;
  }

  /* Create temp directory template with PID for concurrent process safety */
  int needed = snprintf(path_out, path_size, "/tmp/%s_%d_XXXXXX", prefix, getpid());
  if (needed < 0 || (size_t)needed >= path_size) {
    return -1;
  }

  /* mkdtemp modifies the template in-place and returns the path on success */
  if (mkdtemp(path_out) == NULL) {
    return -1;
  }

  return 0;
}

int platform_dir_foreach(const char *path, platform_dir_foreach_cb callback, void *user_data) {
  if (!path || !callback) {
    return -1;
  }

  DIR *dir = opendir(path);
  if (!dir) {
    return -1;
  }

  struct dirent *entry;
  while ((entry = readdir(dir)) != NULL) {
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
      continue;
    }
    platform_dir_entry_t dir_entry = {
        .name = entry->d_name,
        .is_dir = (entry->d_type == DT_DIR),
    };
    if (!callback(&dir_entry, user_data)) {
      break;
    }
  }

  closedir(dir);
  return 0;
}

int platform_rmdir_recursive(const char *path) {
  if (!path) {
    return -1;
  }

  DIR *dir = opendir(path);
  if (!dir) {
    /* Path doesn't exist or isn't a directory - treat as success (no-op) */
    return 0;
  }

  struct dirent *entry;
  int result = 0;

  while ((entry = readdir(dir)) != NULL) {
    /* Skip . and .. */
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
      continue;
    }

    /* Build full path to entry */
    char full_path[PLATFORM_MAX_PATH_LENGTH];
    int len = snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);
    if (len < 0 || len >= (int)sizeof(full_path)) {
continue;
    }

    /* Check if it's a directory using stat */
    struct stat st;
    if (stat(full_path, &st) == 0 && S_ISDIR(st.st_mode)) {
      /* Recursively delete subdirectory */
      int subdir_result = platform_rmdir_recursive(full_path);
      if (subdir_result != 0) {
        result = subdir_result;
      }
    } else {
      /* Delete file */
      if (unlink(full_path) != 0) {
result = EIO;
      }
    }
  }

  closedir(dir);

  /* Delete the directory itself */
  if (rmdir(path) != 0) {
    return -1;
  }

  return result;
}

// ============================================================================
// Key File Security
// ============================================================================

// Group and other permissions mask - keys should only be readable by owner
#define SSH_KEY_PERMISSIONS_MASK (S_IRWXG | S_IRWXO)

int platform_validate_key_file_permissions(const char *key_path) {
  if (!key_path) {
    return -1;
  }

  struct stat st;
  if (stat(key_path, &st) != 0) {
    return -1;
  }

  // Check for overly permissive permissions
  // SSH keys should only be readable by owner (permissions 0600 or 0400)
  if ((st.st_mode & SSH_KEY_PERMISSIONS_MASK) != 0) {
    return EACCES;
  }

  return 0;
}

// ============================================================================
// Config File Search
// ============================================================================

/**
 * @brief Get XDG_CONFIG_HOME directory with fallback to ~/.config
 * @return Allocated string (must be freed by caller)
 */
static char *get_xdg_config_home(void) {
  const char *xdg_config_home = platform_getenv("XDG_CONFIG_HOME");

  if (xdg_config_home && xdg_config_home[0] != '\0') {
    return platform_strdup(xdg_config_home);
  }

  // Fallback: ~/.config
  const char *home = platform_getenv("HOME");
  if (!home) {
    home = "/root";
  }

  char path[PLATFORM_MAX_PATH_LENGTH];
  int ret = snprintf(path, sizeof(path), "%s/.config", home);
  if (ret < 0 || (size_t)ret >= sizeof(path)) {
    return NULL; // Path too long
  }

  return platform_strdup(path);
}

/**
 * @brief Parse XDG_CONFIG_DIRS (colon-separated) into array of directories
 * @param dirs_out Pointer to array (allocated by this function)
 * @param count_out Pointer to count of directories
 * @return 0 on success
 *
 * Default if XDG_CONFIG_DIRS not set: /etc/xdg
 * Caller must free dirs_out array with SAFE_FREE
 */
static int get_xdg_config_dirs(char ***dirs_out, size_t *count_out) {
  if (!dirs_out || !count_out) {
    return -1;
  }

  *dirs_out = NULL;
  *count_out = 0;

  const char *xdg_config_dirs = platform_getenv("XDG_CONFIG_DIRS");
  if (!xdg_config_dirs || xdg_config_dirs[0] == '\0') {
    xdg_config_dirs = "/etc/xdg"; // Default
  }

  // Count colons to determine number of directories
  size_t num_dirs = 1;
  for (const char *p = xdg_config_dirs; *p; p++) {
    if (*p == ':') {
      num_dirs++;
    }
  }

  // Allocate array of directory strings
  char **dirs = malloc(sizeof(char *) * num_dirs);
  if (!dirs) {
    return ENOMEM;
  }

  // Parse the colon-separated list
  char *dirs_copy = platform_strdup(xdg_config_dirs);
  if (!dirs_copy) {
    free(dirs);
    return ENOMEM;
  }

  size_t idx = 0;
  char *saveptr = NULL;
  char *token = strtok_r(dirs_copy, ":", &saveptr);

  while (token && idx < num_dirs) {
    // Skip empty tokens
    if (token[0] != '\0') {
      dirs[idx] = platform_strdup(token);
      if (!dirs[idx]) {
        // Free previously allocated directories on error
        for (size_t i = 0; i < idx; i++) {
          free(dirs[i]);
        }
        free(dirs);
        free(dirs_copy);
        return ENOMEM;
      }
      idx++;
    }
    token = strtok_r(NULL, ":", &saveptr);
  }

  free(dirs_copy);

  *dirs_out = dirs;
  *count_out = idx;
  return 0;
}

/**
 * @brief Find config file across multiple standard locations (POSIX implementation)
 *
 * Search priority (highest to lowest):
 * 1. XDG_CONFIG_HOME/application (default: ~/.config/application)
 * 2. Each directory in XDG_CONFIG_DIRS/application (default: /etc/xdg/application)
 * 3. Backward compatibility paths:
 *    - /opt/homebrew/etc/application (macOS Homebrew ARM)
 *    - /home/linuxbrew/.linuxbrew/etc/application (Linux Homebrew system-wide)
 *    - ~/.linuxbrew/etc/application (Linux Homebrew user)
 *    - /usr/local/etc/application (Unix/Linux local)
 *    - /etc/application (system-wide)
 */
int platform_find_config_file(const char *filename, config_file_list_t *list_out) {
  if (!filename || !list_out) {
    return -1;
  }

  // Initialize output list
  list_out->files = NULL;
  list_out->count = 0;
  list_out->capacity = 0;

  // Get XDG directories
  char *xdg_config_home = get_xdg_config_home();
  if (!xdg_config_home) {
    return -1;
  }

  char **xdg_config_dirs = NULL;
  size_t xdg_config_dirs_count = 0;
  int xdg_result = get_xdg_config_dirs(&xdg_config_dirs, &xdg_config_dirs_count);
  if (xdg_result != 0) {
    free(xdg_config_home);
    return -1;
  }

  // Calculate total number of search directories
  // 1 (XDG_CONFIG_HOME) + xdg_config_dirs_count + 3 (legacy) + 1 (user Linuxbrew on Linux)
  const size_t total_dirs = 1 + xdg_config_dirs_count + 4;

  // Pre-allocate capacity for all possible results
  list_out->capacity = total_dirs;
  list_out->files = malloc(sizeof(config_file_result_t) * total_dirs);
  if (!list_out->files) {
    free(xdg_config_home);
    for (size_t i = 0; i < xdg_config_dirs_count; i++) {
      free(xdg_config_dirs[i]);
    }
    free(xdg_config_dirs);
    return -1;
  }

  uint8_t priority = 0;
  char full_path[PLATFORM_MAX_PATH_LENGTH];

  // Priority 0: XDG_CONFIG_HOME/application (highest priority, user config)
  int ret = snprintf(full_path, sizeof(full_path), "%s/application/%s", xdg_config_home, filename);
  if (ret >= 0 && (size_t)ret < sizeof(full_path)) {
    if (platform_is_regular_file(full_path)) {
      config_file_result_t *result = &list_out->files[list_out->count];
      result->path = platform_strdup(full_path);
      if (result->path) {
        result->priority = priority++;
        result->exists = true;
        result->is_system_config = false; // User config
        list_out->count++;
      }
    }
  }

  // Priorities 1+: XDG_CONFIG_DIRS/application
  for (size_t i = 0; i < xdg_config_dirs_count; i++) {
    ret = snprintf(full_path, sizeof(full_path), "%s/application/%s", xdg_config_dirs[i], filename);
    if (ret >= 0 && (size_t)ret < sizeof(full_path)) {
      if (platform_is_regular_file(full_path)) {
        config_file_result_t *result = &list_out->files[list_out->count];
        result->path = platform_strdup(full_path);
        if (result->path) {
          result->priority = priority++;
          result->exists = true;
          result->is_system_config = true; // System config
          list_out->count++;
        }
      }
    }
  }

  // Backward compatibility: Legacy paths
  const char *legacy_dirs[] = {
#ifdef __APPLE__
      "/opt/homebrew/etc/application", // macOS Homebrew ARM
#else
      "/home/linuxbrew/.linuxbrew/etc/application", // Linux Homebrew (system-wide)
#endif
      "/usr/local/etc/application", // Unix/macOS local
      "/etc/application",           // System-wide
  };
  const size_t num_legacy = sizeof(legacy_dirs) / sizeof(legacy_dirs[0]);

  for (size_t i = 0; i < num_legacy; i++) {
    ret = snprintf(full_path, sizeof(full_path), "%s/%s", legacy_dirs[i], filename);
    if (ret >= 0 && (size_t)ret < sizeof(full_path)) {
      if (platform_is_regular_file(full_path)) {
        config_file_result_t *result = &list_out->files[list_out->count];
        result->path = platform_strdup(full_path);
        if (result->path) {
          result->priority = priority++;
          result->exists = true;
          result->is_system_config = true; // System config
          list_out->count++;
        }
      }
    }
  }

#ifndef __APPLE__
  // Linux: Check user Linuxbrew path (~/.linuxbrew/etc/application)
  const char *home = platform_getenv("HOME");
  if (home && home[0] != '\0') {
    ret = snprintf(full_path, sizeof(full_path), "%s/.linuxbrew/etc/application/%s", home, filename);
    if (ret >= 0 && (size_t)ret < sizeof(full_path)) {
      if (platform_is_regular_file(full_path)) {
        config_file_result_t *result = &list_out->files[list_out->count];
        result->path = platform_strdup(full_path);
        if (result->path) {
          result->priority = priority++;
          result->exists = true;
          result->is_system_config = false; // User-local Homebrew install
          list_out->count++;
        }
      }
    }
  }
#endif

  // Cleanup
  free(xdg_config_home);
  for (size_t i = 0; i < xdg_config_dirs_count; i++) {
    free(xdg_config_dirs[i]);
  }
  free(xdg_config_dirs);

  return 0;
}

/**
 * @brief Free config file list resources (POSIX implementation)
 */
void config_file_list_destroy(config_file_list_t *list) {
  if (!list) {
    -1;
    return;
  }

  if (list->files) {
    for (size_t i = 0; i < list->count; i++) {
      free(list->files[i].path);
    }
    free(list->files);
  }

  list->count = 0;
  list->capacity = 0;
}

// ============================================================================
// Home and Config Directory Discovery (POSIX Implementation)
// ============================================================================

const char *platform_get_home_dir(void) {
  /* Unix: Use HOME environment variable */
  return platform_getenv("HOME");
}

char *platform_get_config_dir(void) {
  /* Unix: Use $XDG_CONFIG_HOME/application/ if set */
  const char *xdg_config_home = platform_getenv("XDG_CONFIG_HOME");
  if (xdg_config_home && xdg_config_home[0] != '\0') {
    size_t len = strlen(xdg_config_home) + strlen("/application/") + 1;
    char *dir = malloc(len);
    if (!dir) {
      return NULL;
    }
    snprintf(dir, len, "%s/application/", xdg_config_home);
    return dir;
  }

  /* Fallback: ~/.config/application/ (XDG Base Directory standard) */
  const char *home = platform_getenv("HOME");
  if (home && home[0] != '\0') {
    size_t len = strlen(home) + strlen("/.config/application/") + 1;
    char *dir = malloc(len);
    if (!dir) {
      return NULL;
    }
    snprintf(dir, len, "%s/.config/application/", home);
    return dir;
  }

  return NULL;
}

char *platform_get_data_dir(void) {
  /* Unix: Use $XDG_DATA_HOME/application/ if set */
  const char *xdg_data_home = platform_getenv("XDG_DATA_HOME");
  if (xdg_data_home && xdg_data_home[0] != '\0') {
    size_t len = strlen(xdg_data_home) + strlen("/application/") + 1;
    char *dir = malloc(len);
    if (!dir) {
      return NULL;
    }
    snprintf(dir, len, "%s/application/", xdg_data_home);
    return dir;
  }

  /* Fallback: ~/.local/share/application/ (XDG Base Directory standard) */
  const char *home = platform_getenv("HOME");
  if (home && home[0] != '\0') {
    size_t len = strlen(home) + strlen("/.local/share/application/") + 1;
    char *dir = malloc(len);
    if (!dir) {
      return NULL;
    }
    snprintf(dir, len, "%s/.local/share/application/", home);
    return dir;
  }

  return NULL;
}

int platform_get_data_dir_candidates(const char *install_prefix, platform_directory_list_t *list_out) {
  (void)install_prefix;
  if (!list_out)
    return -1;
  list_out->candidates = NULL;
  list_out->count = 0;
  return 0;
}

// ============================================================================
// Platform Path Utilities
// ============================================================================

/**
 * @brief Open temporary file (POSIX)
 *
 * On POSIX, platform_create_temp_file already returns a valid fd, so just return it.
 */
int platform_temp_file_open(const char *name, const char *path, int *fd_out) {
  if (!name || !path) {
    return -1;
  }
  if (!fd_out) {
    return -1;
  }
  /* On POSIX, the caller already has the fd from platform_create_temp_file */
  /* This is a no-op wrapper for API consistency */
  return 0;
}

/**
 * @brief Skip absolute path prefix (POSIX)
 *
 * On Unix, absolute paths don't have a drive letter prefix, so return the original path.
 */
const char *platform_path_skip_absolute_prefix(const char *path) {
  /* Unix: No drive letter to skip */
  return path;
}

/**
 * @brief Normalize path separators for the current platform (POSIX)
 *
 * On Unix, forward slashes are already the standard, so this is a no-op.
 */
void platform_normalize_path_separators(char *path) {
  /* Unix: No-op - already uses forward slashes */
  (void)path;
}

/**
 * @brief Platform-aware path string comparison (POSIX)
 *
 * On Unix, paths are case-sensitive, so use standard strncmp.
 */
int platform_path_strcasecmp(const char *a, const char *b, size_t n) {
  /* Unix: Case-sensitive comparison */
  return strncmp(a, b, n);
}

/**
 * @brief Truncate file (POSIX)
 */
int platform_truncate_file(const char *path, size_t size) {
  if (!path) {
    return -1;
  }

  if (truncate(path, (off_t)size) != 0) {
    return -1;
  }

  return 0;
}

/**
 * @brief Check if path is absolute (POSIX)
 */
bool platform_path_is_absolute(const char *path) {
  if (!path || path[0] == '\0') {
    return false;
  }
  /* POSIX: Absolute if starts with / */
  return path[0] == '/';
}

/**
 * @brief Get path separator (POSIX)
 */
char platform_path_get_separator(void) {
  return '/';
}

/**
 * @brief Normalize path (POSIX)
 */
int platform_path_normalize(const char *input, char *output, size_t output_size) {
  if (!input || !output || output_size == 0) {
    return -1;
  }

  size_t input_len = strlen(input);
  if (input_len >= output_size) {
    return -1;
  }

  /* POSIX: Copy and remove redundant slashes */
  size_t output_pos = 0;
  bool last_was_slash = false;

  for (size_t i = 0; i < input_len; i++) {
    char c = input[i];
    if (c == '/') {
      if (!last_was_slash) {
        output[output_pos++] = c;
        last_was_slash = true;
      }
    } else {
      output[output_pos++] = c;
      last_was_slash = false;
    }
  }

  /* Remove trailing slash unless it's the root */
  if (output_pos > 1 && output[output_pos - 1] == '/') {
    output_pos--;
  }

  output[output_pos] = '\0';
  return 0;
}

// ============================================================================
// Binary PATH Cache - POSIX Implementation
// ============================================================================

/**
 * @brief Check if a file exists and is executable (POSIX)
 */
static bool is_executable_file(const char *path) {
  return (access(path, X_OK) == 0);
}

/**
 * @brief Check if binary is in PATH (no caching) - POSIX implementation
 */
bool check_binary_in_path_uncached(const char *bin_name) {
  char bin_with_suffix[PLATFORM_POSIX_FILESYSTEM_BIN_WITH_SUFFIX_BUFFER_SIZE];
  char full_path[PLATFORM_MAX_PATH_LENGTH];

  strncpy(bin_with_suffix, bin_name, sizeof(bin_with_suffix));

  const char *path_env = platform_getenv("PATH");
  if (!path_env) {
    return false;
  }

  size_t path_len = strlen(path_env);
  char *path_copy = malloc(path_len + 1);
  if (!path_copy) {
    return false;
  }
  strncpy(path_copy, path_env, path_len + 1);

  bool found = false;
  char *saveptr = NULL;
  const char *separator = PATH_ENV_SEPARATOR;

  char *dir = platform_strtok_r(path_copy, separator, &saveptr);

  while (dir != NULL) {
    if (dir[0] == '\0') {
      dir = platform_strtok_r(NULL, separator, &saveptr);
      continue;
    }

    snprintf(full_path, sizeof(full_path), "%s%c%s", dir, PATH_DELIM, bin_with_suffix);

    if (is_executable_file(full_path)) {
      found = true;
      break;
    }

    dir = platform_strtok_r(NULL, separator, &saveptr);
  }

  free(path_copy);
  return found;
}

bool platform_is_binary_in_path(const char *bin_name) {
  return bin_name && bin_name[0] != '\0' && check_binary_in_path_uncached(bin_name);
}

#ifdef __linux__
bool platform_get_executable_path(char *exe_path, size_t path_size) {
  if (!exe_path || path_size < 2) return false;
  ssize_t len = readlink("/proc/self/exe", exe_path, path_size - 1);
  if (len < 0 || (size_t)len >= path_size - 1) return false;
  exe_path[len] = '\0';
  return true;
}
#elif defined(__APPLE__)
bool platform_get_executable_path(char *exe_path, size_t path_size) {
  if (!exe_path || path_size < 2 || path_size > UINT32_MAX) return false;
  uint32_t length = (uint32_t)path_size;
  if (_NSGetExecutablePath(exe_path, &length) != 0 || length >= path_size) return false;
  exe_path[length] = '\0';
  return true;
}
#endif

/**
 * @brief Synchronize file descriptor to disk
 * @param fd File descriptor to sync
 * @return fsync() return result
 */
int platform_fsync(int fd) {
  // Windows uses _commit for file sync
  return fsync(fd);
}

#endif
