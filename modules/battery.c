#include "battery.h"
#include "../defs.h"
#include "../display.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct {
  int capacity_threshold;
  char *icon;
} bat_status;

// TODO -> read those values from config file
#define BATTERY_PATH "/sys/class/power_supply/"
#define BATTERY "BAT0"

bat_status batList[] = {
    {90, "󰁹"}, // >= 90%
    {80, "󰂂"}, // >= 80%
    {70, "󰂁"}, // >= 70%
    {60, "󰂀"}, // >= 60%
    {50, "󰁿"}, // >= 50%
    {40, "󰁾"}, // >= 40%
    {30, "󰁽"}, // >= 30%
    {20, "󰁼"}, // >= 20%
    {10, "󰁻"}, // >= 10%
    {5, "󰁺"},  // >= 5%
    {-1, "󰂎"}, // >= 0%
};

void *battery_update(void *) {
  pthread_mutex_trylock(&mutex);
  modules[battery].string = (char *)malloc((BATTERY_BUFFER * sizeof(char)));
  modules[battery].string[0] = '\0';
  pthread_mutex_unlock(&mutex);

  char bat_path[50];
  strcat(bat_path, BATTERY_PATH);
  strcat(bat_path, BATTERY);
  FILE *fbat_cap;
  FILE *fbat_status;
  char bat_cap[5];
  char bat_status[20];
  char bat_path_tmp[50];
  char bat_path_tmp_2[50];

  strcpy(bat_path_tmp, bat_path);
  if ((fbat_cap = fopen(strcat(bat_path_tmp, "/capacity"), "r")) == NULL) {
    printf("Failed to read battery capacity.");
    exit(0);
  }
  strcpy(bat_path_tmp_2, bat_path);
  if ((fbat_status = fopen(strcat(bat_path_tmp_2, "/status"), "r")) == NULL) {
    printf("Failed to read battery status.");
    exit(0);
  }

  int previous_battery_capacity = -1;

  while (running) {
    fgets(bat_cap, sizeof(bat_cap), fbat_cap);
    fgets(bat_status, sizeof(bat_status), fbat_status);
    int capacity = atoi(bat_cap);
    if (capacity != previous_battery_capacity) {

      remove_nl(bat_status);
      if (strcmp(bat_status, "Charging")) {
        for (int i = 0; i < (int)(sizeof(batList) / (sizeof(bat_status)));
             i++) {
          if (capacity > batList[i].capacity_threshold) {
            sprintf(modules[battery].string, "%s %d%%", batList[i].icon,
                    capacity);
            break;
          }
        }
      } else {
        sprintf(modules[battery].string, "󰂄 %d%%", capacity);
      }
      display_modules(modules[battery].position);
      previous_battery_capacity = capacity;
    }
    fflush(fbat_cap);
    fflush(fbat_status);
    rewind(fbat_cap);
    rewind(fbat_status);
    sleep(modules[battery].interval);
  }
  fclose(fbat_cap);
  fclose(fbat_status);
  free(modules[battery].string);
  return NULL;
}