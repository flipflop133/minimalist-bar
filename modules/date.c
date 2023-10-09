#include "date.h"
#include "../defs.h"
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include "../display.h"
void *date_update(void *) {
  int first_iteration = 1;

  while (running) {
    pthread_mutex_trylock(&mutex);
    modules[date].string = (char *)malloc((DATE_BUFFER + 1 * sizeof(char)));
    modules[date].string[0] = '\0';
    pthread_mutex_unlock(&mutex);
    time_t t = time(&t);
    const struct tm *my_time_props = localtime(&t);
    strftime(modules[date].string, DATE_BUFFER, "%a %d %I:%M", my_time_props);
    display_modules(modules[date].position);
    sleep(first_iteration ? (60 - (my_time_props->tm_sec)) : 60);
    first_iteration = 0;
  }

  free(modules[date].string);
  return NULL;
}