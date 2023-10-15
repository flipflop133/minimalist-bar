#include "date.h"
#include "../defs.h"

#include "../display.h"
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <stdio.h>
void *date_update(void *arg) {
  int first_iteration = 1;
  struct Module *module = (struct Module *)arg;
  while (running) {
    pthread_mutex_lock(&mutex);
    module->string = (char *)malloc((DATE_BUFFER + 1 * sizeof(char)));
    module->string[0] = '\0';
    pthread_mutex_unlock(&mutex);
    time_t t = time(&t);
    const struct tm *my_time_props = localtime(&t);
    strftime(module->string, DATE_BUFFER, "%a %d %I:%M", my_time_props);
    module->string[13] = '\0';
    printf("date calling display_modules\n");
    printf("date position: %d\n", module->position);
    display_modules(module->position);
    sleep(first_iteration ? (60 - (my_time_props->tm_sec)) : 60);
    first_iteration = 0;
  }

  free(module->string);
  return NULL;
}