#include "parser.h"
#include "defs.h"
#include <cjson/cJSON.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>
static void parse_modules(cJSON *modules_json);
static void parse_options(cJSON *json);

struct Config_files
{
  char *strings[256];
  int used;
} config_files;

void addPath(struct Config_files *config, const char *path)
{
  config->strings[config->used] = strdup(path);
  strcat(config->strings[config->used], "\0");
  config->used++;
}

void freePaths(struct Config_files *config)
{
  for (int i = 0; i < config->used; ++i)
  {
    free(config->strings[i]);
  }
  config->used = 0;
}

void parse_config(void)
{
  // Read config file
  char config[4096] = {'\0'};
  FILE *config_file;
  char *config_file_path = retrieve_command_arg("--config");

  // If config file was not given as an argument, read it from default path.
  if (config_file_path == NULL)
  {
    struct passwd *info = getpwuid(1000);
    config_file_path = malloc(
        sizeof(info->pw_dir) * sizeof(char) +
        sizeof(CONFIG_PATH) * sizeof(char));
    strcpy(config_file_path, info->pw_dir);
    strcat(config_file_path, CONFIG_PATH);
  }

  config_file = fopen(config_file_path, "r");

  if (config_file == NULL)
  {
    printf("Can't read %s, make sure the file exists.\n", config_file_path);
    exit(1);
  }

  // Read base config file
  char buf[1024] = {'\0'};
  while (fgets(buf, sizeof(buf), config_file))
  {
    strcat(config, buf);
  }
  fclose(config_file);

  addPath(&config_files, config);

  // Parse json from config file
  cJSON *json = cJSON_Parse(config);
  cJSON *include_json =
      cJSON_GetObjectItemCaseSensitive(json, "include");
  if (include_json != NULL)
  {
    // Retrieve directory from config file path
    const char *lastSlash = strrchr(config_file_path, '/');
    size_t pathLength = lastSlash - config_file_path + 1;
    char directoryPath[pathLength + 1];
    strncpy(directoryPath, config_file_path, pathLength);
    directoryPath[pathLength] = '\0';

    strcat(directoryPath, include_json->valuestring);

    config_file = fopen(directoryPath, "r");
    if (config_file == NULL)
    {
      printf("Can't read included file %s, make sure the file exists.\n", directoryPath);
      exit(1);
    }
    // Read included config file
    char buf[1024] = {'\0'};
    char included_config[1024] = {'\0'};
    while (fgets(buf, sizeof(buf), config_file))
    {
      strcat(included_config, buf);
    }
    addPath(&config_files, included_config);
    fclose(config_file);
  }
  free(config_file_path);
  for (int i = 0; i < config_files.used; i++)
  {
    json = cJSON_Parse(config_files.strings[i]);
    // Parse modules
    cJSON *modules_json = cJSON_GetObjectItemCaseSensitive(json, "modules");

    if (modules_json != NULL)
    {
      modules_json = modules_json->child;
      int number_of_modules = 0;
      cJSON *tmp = modules_json;
      while (tmp != NULL)
      {
        tmp = tmp->next;
        number_of_modules++;
      }
      displayOrder.list = (int *)malloc(sizeof(int) * number_of_modules);

      parse_modules(modules_json);
    }

    // Parse config options
    cJSON *options_json = cJSON_GetObjectItemCaseSensitive(json, "general");
    if (options_json != NULL)
    {
      parse_options(options_json);
    }
    cJSON_Delete(json);
  }

  freePaths(&config_files);
}

static void parse_modules(cJSON *modules_json)
{
  // Set modules values from config file
  int first = 1;
  struct Module *previous = NULL;
  struct Module *current = NULL;
  while (modules_json != NULL)
  {
    current = (struct Module *)malloc(sizeof(struct Module));

    // Network module
    if (strcmp("network", modules_json->string) == 0)
    {
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
      current->string = (char *)malloc((NETWORK_BUFFER * sizeof(char)));
      strcpy(current->string, "\0");
    }

    // Date module
    else if (strcmp("date", modules_json->string) == 0)
    {
      struct Date *options = (struct Date *)malloc(sizeof(struct Date));
      char *format =
          cJSON_GetObjectItemCaseSensitive(modules_json, "format")
              ->valuestring;
      options->format = (char *)malloc(sizeof(char) * strlen(format));
      strcpy(options->format, format);
      current->Module_infos = options;
      current->thread_function = date_update;
      strcpy(current->name, "date");
      current->string = (char *)malloc((DATE_BUFFER * sizeof(char)));
      strcpy(current->string, "\0");
    }

    // Battery module
    else if (strcmp("battery", modules_json->string) == 0)
    {
      struct Battery *options =
          (struct Battery *)malloc(sizeof(struct Battery));
      char *battery = cJSON_GetObjectItemCaseSensitive(modules_json, "battery")
                          ->valuestring;
      current->thread_function = battery_update;
      options->battery = (char *)malloc(sizeof(char) * strlen(battery));
      strcpy(options->battery, battery);
      current->Module_infos = options;
      strcpy(current->name, "battery");
      current->string = (char *)malloc((BATTERY_BUFFER * sizeof(char)));
      strcpy(current->string, "\0");
    }

    // Media module
    else if (strcmp("media", modules_json->string) == 0)
    {
      struct Media *options =
          (struct Media *)malloc(sizeof(struct Media));
      options->title_max_length = cJSON_GetObjectItemCaseSensitive(modules_json, "title-max-length")
                                      ->valueint;
      options->artist_max_length = cJSON_GetObjectItemCaseSensitive(modules_json, "artist-max-length")
                                       ->valueint;
      current->Module_infos = options;
      current->thread_function = media_update;
      strcpy(current->name, "media");
      current->string = (char *)malloc((MEDIA_BUFFER * sizeof(char)));
      strcpy(current->string, "\0");
    }
    // Bluetooth module
    else if (strcmp("bluetooth", modules_json->string) == 0)
    {
      current->thread_function = bluetooth_update;
      strcpy(current->name, "bluetooth");
      current->string = (char *)malloc((BLUETOOTH_BUFFER * sizeof(char)));
      strcpy(current->string, "\0");
    }
    // Volume module
    else if (strcmp("volume", modules_json->string) == 0)
    {
      current->thread_function = volume_update;
      strcpy(current->name, "volume");
      current->string = (char *)malloc((VOLUME_BUFFER * sizeof(char)));
      strcpy(current->string, "\0");
    }
    // Mic module
    else if (strcmp("mic", modules_json->string) == 0)
    {
      current->thread_function = mic_update;
      strcpy(current->name, "mic");
      current->string = (char *)malloc((MIC_BUFFER * sizeof(char)));
      strcpy(current->string, "\0");
    }
    // Unknow module
    else
    {
      fprintf(stderr, "Error in config file, module %s doesn't exist!\n",
              modules_json->string);
      exit(1);
    }

    cJSON *json_position =
        cJSON_GetObjectItemCaseSensitive(modules_json, "position");

    if (json_position != NULL)
    {
      char *position = json_position->valuestring;
      if (strcmp(position, "left") == 0)
      {
        current->position = LEFT;
      }
      else if (strcmp(position, "center") == 0)
      {
        current->position = CENTER;
      }
      else
      {
        current->position = RIGHT;
      }
    }
    else
    {
      current->position = RIGHT;
    }
    if (first)
    {
      head = current;
      first = 0;
    }
    else
    {
      previous->next = current;
    }
    previous = current;
    current->next = NULL;
    modules_json = modules_json->next;
    modules_count++;
  }
}

// TODO remove redundancy of this function
static void parse_options(cJSON *options_json)
{
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
  strcpy(options.workspace_foreground_color,
         (cJSON_GetObjectItemCaseSensitive(options_json, "workspace-foreground-color")
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