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

// ModuleInfo modules[] = {{.name = "date",
//                          .thread_function = date_update,
//                          .enabled = 0,
//                          .string = NULL},
//                         {.name = "network",
//                          .thread_function = wifi_update,
//                          .enabled = 0,
//                          .string = NULL},
//                         {.name = "bluetooth",
//                          .thread_function = bluetooth_update,
//                          .enabled = 0,
//                          .string = NULL},
//                         {.name = "volume",
//                          .thread_function = volume_update,
//                          .enabled = 0,
//                          .string = NULL},
//                         {.name = "mic",
//                          .thread_function = mic_update,
//                          .enabled = 0,
//                          .string = NULL},
//                         {.name = "media",
//                          .thread_function = media_update,
//                          .enabled = 0,
//                          .string = NULL,
//                          .position = CENTER},
//                         {.name = "battery",
//                          .thread_function = battery_update,
//                          .enabled = 0,
//                          .string = NULL,
//                          .interval = 5}};
DisplayOrder displayOrder;
Options options;
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

void *launchModules(void *) {
  // Start i3 thread
  pthread_t i3_thread;
  pthread_create(&i3_thread, NULL, listen_to_i3, NULL);

  // Start modules threads
  // pthread_t threads[sizeof(modules) / sizeof(modules[0])];
  // for (size_t i = 0; i < sizeof(modules) / sizeof(modules[0]); ++i) {
  //   if (modules[i].enabled) {
  //     printf("starting module: %s\n", modules[i].name);
  //     pthread_create(&threads[i], NULL, modules[i].thread_function, NULL);
  //   }
  // }
  // Wait for modules threads
  // for (size_t i = 0; i < sizeof(modules) / sizeof(modules[0]); ++i) {
  //   if (modules[i].enabled) {
  //     pthread_join(threads[i], NULL);
  //   }
  // }

  // Wait for i3 thread
  pthread_join(i3_thread, NULL);

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
  cJSON *modules_json =
      cJSON_GetObjectItemCaseSensitive(json, "modules")->child;
  int number_of_modules = 0;
  cJSON *tmp = modules_json;
  while (tmp != NULL) {
    tmp = tmp->next;
    number_of_modules++;
  }
  displayOrder.list = (int *)malloc(sizeof(int) * number_of_modules);

  // Set modules values from config file
  int first = 1;
  while (modules_json != NULL) {
    current = (struct Module *)malloc(sizeof(struct Module));

    if (strcmp("Network", modules_json->string) == 0) {
      Network options;
      current->thread_function = wifi_update;
      char *interface =
          cJSON_GetObjectItemCaseSensitive(modules_json, "interface")
              ->valuestring;
      options.interface = malloc(sizeof(char) * strlen(interface));
      strcpy(options.interface, interface);
      printf("Module name: %s\n", options.interface);
      current->Module_infos = &options;
      if(first){
        head = current;
      }
      current->next = NULL;
      current = current->next;
      
    }
    if (strcmp("Date", modules_json->string) == 0) {
      Date options;
      current->thread_function = date_update;
      current->Module_infos = &options;
      if(first){
        head = current;
      }
      current->next = NULL;
      current = current->next;
    } else {
      fprintf(stderr, "Error in config file, module %s doesn't exist!\n",
              modules_json->string);
      exit(1);
    }
    modules_json = modules_json->next;
  }

  // Parse config options
  cJSON *options_json = cJSON_GetObjectItemCaseSensitive(json, "general");
  // Colors
  strcpy(options.background_color,
         (cJSON_GetObjectItemCaseSensitive(options_json, "background-color")
              ->valuestring));
  strcpy(options.foreground_color,
         (cJSON_GetObjectItemCaseSensitive(options_json, "foreground-color")
              ->valuestring));
  strcpy(options.workspace_color,
         (cJSON_GetObjectItemCaseSensitive(options_json, "workspace-color")
              ->valuestring));
  strcpy(
      options.workspace_color_urgent,
      (cJSON_GetObjectItemCaseSensitive(options_json, "workspace-color-urgent")
           ->valuestring));
  // Font
  char *font_name =
      cJSON_GetObjectItemCaseSensitive(options_json, "font-name")->valuestring;
  options.font_name = malloc(sizeof(char) * strlen(font_name));
  strcpy(options.font_name, font_name);
  options.font_size =
      cJSON_GetObjectItemCaseSensitive(options_json, "font-size")->valueint;
  // Sizes
  options.right_padding =
      cJSON_GetObjectItemCaseSensitive(options_json, "right-padding")->valueint;
  options.module_left_padding =
      cJSON_GetObjectItemCaseSensitive(options_json, "module-left-padding")
          ->valueint;
  options.bar_height =
      cJSON_GetObjectItemCaseSensitive(options_json, "bar-height")->valueint;
  options.workspace_padding =
      cJSON_GetObjectItemCaseSensitive(options_json, "workspace-padding")
          ->valueint;
  // Other
  strcpy(options.bar_position,
         (cJSON_GetObjectItemCaseSensitive(options_json, "bar-position")
              ->valuestring));
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