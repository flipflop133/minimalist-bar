#include "media.h"
#include "../defs.h"
#include <stdio.h>
#include <stdlib.h>
#include "../display.h"
void *media_update(void *arg) {
  struct Module *module = (struct Module *)arg;
  pthread_mutex_lock(&mutex);
  module->string = (char *)malloc((MEDIA_BUFFER+1 * sizeof(char)));
  module->string[0] = '\0';
  pthread_mutex_unlock(&mutex);
  FILE *fp = popen("playerctl -Fa metadata --format '{{trunc(artist,15)}} - "
                   "{{trunc(title,15)}}'",
                   "r");
                   // TODO add maxlen to config file

  if (fp != NULL) {
    while ((fgets(module->string, MEDIA_BUFFER, fp) != NULL) && running) {
      remove_nl(module->string);
      display_modules(module->position);
    }
    pclose(fp);
  }
  free(module->string);
  return NULL;
}