#include "date.h"
#include "../defs.h"
#include "../display.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

void *date_update(void *arg) {
  char previous_string[DATE_BUFFER] = "\0";
  struct Module *module = (struct Module *)arg;
  pthread_mutex_lock(&mutex);
  module->string = (char *)malloc((DATE_BUFFER + 1 * sizeof(char)));
  pthread_mutex_unlock(&mutex);
  while (running) {
    time_t t = time(&t);
    const struct tm *my_time_props = localtime(&t);
    strftime(module->string, DATE_BUFFER, "%a %d %I:%M", my_time_props);

    // This avoid calling display_modules too much
    if (strcmp(module->string, previous_string) != 0) {
      display_modules(module->position);
    }
    strcpy(previous_string, module->string);
    sleep(1); // TODO sleep longer, taking resume/suspend into account
  }

  free(module->string);
  return NULL;
}