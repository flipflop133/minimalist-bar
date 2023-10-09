#include "media.h"
#include "../defs.h"
#include <stdio.h>
#include <stdlib.h>
#include "../display.h"
void *media_update(void *) {
  pthread_mutex_lock(&mutex);
  modules[media].string = (char *)malloc((MEDIA_BUFFER+1 * sizeof(char)));
  modules[media].string[0] = '\0';
  pthread_mutex_unlock(&mutex);
  FILE *fp = popen("playerctl -Fa metadata --format '{{trunc(artist,15)}} - "
                   "{{trunc(title,15)}}'",
                   "r");

  if (fp != NULL) {
    while ((fgets(modules[media].string, MEDIA_BUFFER, fp) != NULL) && running) {
      remove_nl(modules[media].string);
      display_modules(modules[media].position);
    }
    pclose(fp);
  }
  free(modules[media].string);
  return NULL;
}