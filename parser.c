#include "parser.h"
#include "defs.h"
#include <cjson/cJSON.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
static void parse_modules(cJSON *modules_json);
static void parse_options(cJSON *json);

void parse_config(void) {
  // Read config file
  char config[4096] = {'\0'};
  FILE *config_file;
  char config_file_path[1024];
  if (readlink("/proc/self/exe", config_file_path,
               sizeof(config_file_path) - 1) == -1) {
    printf("Failed to read config file path.");
    exit(1);
  }
  printf("ok\n");
  char* directory = retrieve_command_arg("--config");
  printf("ko\n");
  printf("%s\n", directory);
  if(strlen(directory) == 0){
    printf("directory len is 0\n");
    directory = dirname(config_file_path);
    strcat(directory, "/i3status.json");
  }
  
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

  parse_modules(modules_json);
  parse_options(json);
  cJSON_Delete(json);
}

static void parse_modules(cJSON *modules_json) {
  // Set modules values from config file
  int first = 1;
  struct Module *previous = NULL;
  struct Module *current = NULL;
  while (modules_json != NULL) {
    current = (struct Module *)malloc(sizeof(struct Module));

    // Network module
    if (strcmp("network", modules_json->string) == 0) {
      struct Network *options =
          (struct Network *)malloc(sizeof(struct Network));
      current->thread_function = wifi_update;
      char *interface =
          cJSON_GetObjectItemCaseSensitive(modules_json, "interface")
              ->valuestring;
      options->interface = (char *)malloc(sizeof(char) * strlen(interface));
      strcpy(options->interface, interface);
      current->Module_infos = options;
      strcpy(current->name, "network");
    }

    // Date module
    else if (strcmp("date", modules_json->string) == 0) {
      struct Date *options = (struct Date *)malloc(sizeof(struct Date));
      char *format =
          cJSON_GetObjectItemCaseSensitive(modules_json, "format")
              ->valuestring;
      options->format = (char*) malloc(sizeof(char) * strlen(format));
      strcpy(options->format, format);
      current->Module_infos = options;
      current->thread_function = date_update;
      strcpy(current->name, "date");
    }

    // Battery module
    else if (strcmp("battery", modules_json->string) == 0) {
      struct Battery *options =
          (struct Battery *)malloc(sizeof(struct Battery));
      char *battery = cJSON_GetObjectItemCaseSensitive(modules_json, "battery")
                          ->valuestring;
      current->thread_function = battery_update;
      options->battery = (char *)malloc(sizeof(char) * strlen(battery));
      strcpy(options->battery, battery);
      current->Module_infos = options;
      strcpy(current->name, "battery");
    }

    // Media module
    else if (strcmp("media", modules_json->string) == 0) {
       struct Media *options =
          (struct Media *)malloc(sizeof(struct Media));      
      options->title_max_length = cJSON_GetObjectItemCaseSensitive(modules_json, "title-max-length")
                          ->valueint;
      options->artist_max_length = cJSON_GetObjectItemCaseSensitive(modules_json, "artist-max-length")
                          ->valueint;
      current->Module_infos = options;
      current->thread_function = media_update;
      strcpy(current->name, "media");

    }
    // Bluetooth module
    else if (strcmp("bluetooth", modules_json->string) == 0) {
      current->thread_function = bluetooth_update;
      strcpy(current->name, "bluetooth");
    }
    // Volume module
    else if (strcmp("volume", modules_json->string) == 0) {
      current->thread_function = volume_update;
      strcpy(current->name, "volume");
    }
    // Mic module
    else if (strcmp("mic", modules_json->string) == 0) {
      current->thread_function = mic_update;
      strcpy(current->name, "mic");
    }
    // Unknow module
    else {
      fprintf(stderr, "Error in config file, module %s doesn't exist!\n",
              modules_json->string);
      exit(1);
    }

    cJSON *json_position =
        cJSON_GetObjectItemCaseSensitive(modules_json, "position");

    if (json_position != NULL) {
      char *position = json_position->valuestring;
      if (strcmp(position, "left") == 0) {
        current->position = LEFT;
      } else if (strcmp(position, "center") == 0) {
        current->position = CENTER;
      } else {
        current->position = RIGHT;
      }
    } else {
      current->position = RIGHT;
    }
    if (first) {
      head = current;
      first = 0;
    } else {
      previous->next = current;
    }
    previous = current;
    current->next = NULL;
    modules_json = modules_json->next;
    modules_count++;
  }
}

// TODO remove redunduncy of this function
static void parse_options(cJSON *json) {
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
}