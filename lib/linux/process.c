#include <libplatform/process.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int platform_process_list(const char *name_substring, platform_process_list_callback_t callback, void *context) {
  if (!callback) return -1;
  DIR *proc = opendir("/proc");
  if (!proc) return -1;
  struct dirent *entry;
  while ((entry = readdir(proc))) {
    char *end = NULL;
    unsigned long value = strtoul(entry->d_name, &end, 10);
    if (!end || *end || !value) continue;
    char path[PLATFORM_PROCESS_NAME_MAX + 32], name[PLATFORM_PROCESS_NAME_MAX];
    snprintf(path, sizeof(path), "/proc/%s/comm", entry->d_name);
    FILE *comm = fopen(path, "r");
    if (!comm || !fgets(name, sizeof(name), comm)) { if (comm) fclose(comm); continue; }
    fclose(comm);
    name[strcspn(name, "\r\n")] = '\0';
    if (!platform_process_name_matches(name, name_substring)) continue;
    platform_process_info_t info = {.pid = (pid_t)value};
    strncpy(info.name, name, sizeof(info.name) - 1);
    if (callback(&info, context)) break;
  }
  closedir(proc);
  return 0;
}
