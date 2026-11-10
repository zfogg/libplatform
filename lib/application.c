#include <libplatform/filesystem.h>
#include <stddef.h>

static const platform_application_config_t default_config = {
    .name = "application",
    .config_directory = "application",
    .data_directory = "application",
};
static platform_application_config_t current_config;
static int configured;

int platform_set_application_config(const platform_application_config_t *config) {
  if (!config || !config->name || !config->config_directory || !config->data_directory ||
      !*config->name || !*config->config_directory || !*config->data_directory) {
    return -1;
  }
  current_config = *config;
  configured = 1;
  return 0;
}

const platform_application_config_t *platform_application_config(void) {
  return configured ? &current_config : &default_config;
}
