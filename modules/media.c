#include "media.h"
#include "../defs.h"
#include "../display.h"
#include <stdio.h>
#include <stdlib.h>
#define COMMAND_BUFFER 100

void *media_update(void *arg) {
  struct Module *module = (struct Module *)arg;
  pthread_mutex_lock(&mutex);
  module->string = (char *)malloc((MEDIA_BUFFER + 1 * sizeof(char)));
  pthread_mutex_unlock(&mutex);
  char command[COMMAND_BUFFER];
  sprintf(command,
          "playerctl -Fa metadata --format '{{trunc(artist,%d)}} - "
          "{{trunc(title,%d)}}'",
          ((struct Media *)(module->Module_infos))->artist_max_length,
          ((struct Media *)(module->Module_infos))->title_max_length);
  FILE *fp = popen(command, "r");
  if (fp != NULL) {
    char tmp[MEDIA_BUFFER];
    while ((fgets(tmp, MEDIA_BUFFER, fp) != NULL) && running) {
      module->string = tmp;
      remove_nl(module->string);
      display_modules(module->position);
    }
    pclose(fp);
  }
  free(module->string);
  return NULL;
}