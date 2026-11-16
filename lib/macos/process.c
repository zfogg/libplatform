#include <libplatform/process.h>
#include <stdio.h>
#include <string.h>

int platform_process_list(const char *name_substring, platform_process_list_callback_t callback, void *context) {
  if (!callback) return -1;
  FILE *pipe = popen("ps -axo pid=,comm=", "r");
  if (!pipe) return -1;
  char line[PLATFORM_PROCESS_NAME_MAX + 32], name[PLATFORM_PROCESS_NAME_MAX];
  while (fgets(line, sizeof(line), pipe)) {
    unsigned long value = 0;
    if (sscanf(line, "%lu %259s", &value, name) == 2 && value && platform_process_name_matches(name, name_substring)) {
      platform_process_info_t info = {.pid = (pid_t)value};
      strncpy(info.name, name, sizeof(info.name) - 1);
      if (callback(&info, context)) break;
    }
  }
  pclose(pipe);
  return 0;
}
