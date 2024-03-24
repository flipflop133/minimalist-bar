#include "date.h"
#include "../defs.h"
#include "../display/display.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

void *date_update(void *arg) {
  char previous_string[DATE_BUFFER] = "\0";

  struct Module *module = (struct Module *)arg;

  while (running) {
    time_t t = time(&t);
    const struct tm *my_time_props = localtime(&t);

    strftime(module->string, DATE_BUFFER, ((struct Date*)(module->Module_infos))->format, my_time_props);
    // This avoid calling display_modules too much
    int val = strcmp(module->string, previous_string);
    if (val != 0) {
      display_modules(module->position);
    }
    strcpy(previous_string, module->string);
    sleep(1); // TODO sleep longer, taking resume/suspend into account
  }

  free(module->string);
  return NULL;
}