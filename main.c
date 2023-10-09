#include "defs.h"
#include "display.h"
#include "i3ipc.h"
#include "modules/battery.h"
#include "modules/bluetooth.h"
#include "modules/date.h"
#include "modules/media.h"
#include "modules/network.h"
#include "modules/volume.h"
#include <cjson/cJSON.h>
#include <libgen.h>
#include <pulse/pulseaudio.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
static void parse_config(void);

ModuleInfo modules[] = {{.name = "date",
                         .thread_function = date_update,
                         .enabled = 0,
                         .string = NULL},
                        {.name = "network",
                         .thread_function = wifi_update,
                         .enabled = 0,
                         .string = NULL},
                        {.name = "bluetooth",
                         .thread_function = bluetooth_update,
                         .enabled = 0,
                         .string = NULL},
                        {.name = "volume",
                         .thread_function = volume_update,
                         .enabled = 0,
                         .string = NULL},
                        {.name = "mic",
                         .thread_function = mic_update,
                         .enabled = 0,
                         .string = NULL},
                        {.name = "media",
                         .thread_function = media_update,
                         .enabled = 0,
                         .string = NULL,
                         .position = CENTER},
                        {.name = "battery",
                         .thread_function = battery_update,
                         .enabled = 0,
                         .string = NULL,
                         .interval = 5}};

int displayOrder[7] = {-1, -1, -1, -1, -1, -1, -1};

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

int running = 1;

int main() {
  // Parse config file
  parse_config();

  signal(SIGTERM, cleanup);
  signal(SIGINT, cleanup);

  // Start display thread
  pthread_t display_thread;
  pthread_create(&display_thread, NULL, display_graphic_bar, NULL);

  // Wait for display thread
  pthread_join(display_thread, NULL);

  return 0;
}

void* launchModules(void*) {
  // Start i3 thread
  pthread_t i3_thread;
  pthread_create(&i3_thread, NULL, listen_to_i3, NULL);

  // Start modules threads
  pthread_t threads[sizeof(modules) / sizeof(modules[0])];
  for (size_t i = 0; i < sizeof(modules) / sizeof(modules[0]); ++i) {
    if (modules[i].enabled) {
      pthread_create(&threads[i], NULL, modules[i].thread_function, NULL);
    }
  }
  // Wait for i3 thread
  pthread_join(i3_thread, NULL);

  // Wait for modules threads
  for (size_t i = 0; i < sizeof(modules) / sizeof(modules[0]); ++i) {
    if (modules[i].enabled) {
      pthread_join(threads[i], NULL);
    }
  }
  return NULL;
}

void cleanup(int) {
  running = 0;
  pa_mainloop_quit(volume_loop, 0);
  pa_mainloop_quit(mic_loop, 0);
}

static void parse_config(void) {
  // Read config file
  char config[4096] = {'\0'};
  FILE *config_file;
  char config_file_path[1024];
  if (readlink("/proc/self/exe", config_file_path,
               sizeof(config_file_path) - 1) == -1) {
    printf("Failed to read config file path.");
    exit(1);
  }
  char *directory = dirname(config_file_path);
  strcat(directory, "/i3status.json");
  config_file = fopen(directory, "r");
  if (config_file == NULL) {
    exit(1);
  }
  char buf[1024] = {'\0'};
  while (fgets(buf, sizeof(buf), config_file)) {
    strcat(config, buf);
  }
  fclose(config_file);

  // Parse json from config file
  cJSON *json = cJSON_Parse(config);
  const cJSON *modules_json = cJSON_GetObjectItemCaseSensitive(json, "modules");
  cJSON *current = modules_json->child;
  int c = 0;
  // Set modules values from config file
  while (current != NULL) {
    for (int i = 0; i < (int)(sizeof(modules) / sizeof(modules[0])); ++i) {
      if (strcmp(modules[i].name, current->string) == 0) {
        modules[i].enabled = 1;
        displayOrder[c] = i;
        break;
      }
    }

    c++;
    current = current->next;
  }

  cJSON_Delete(json);
}

int remove_nl(char *str) {
  for (int i = 0; str[i] != '\0'; i++) {
    if (str[i] == '\n') {
      str[i] = '\0';
      return 0;
    }
  }
  return 1;
}