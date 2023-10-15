#include "defs.h"
#include "display.h"
#include "i3ipc.h"
#include "modules/battery.h"
#include "modules/bluetooth.h"
#include "modules/date.h"
#include "modules/media.h"
#include "modules/network.h"
#include "modules/volume.h"
#include "parser.h"
#include <pulse/pulseaudio.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>

DisplayOrder displayOrder;
Options options;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
struct Module *head = NULL;
int modules_count = 0;
int running = 1;

int main() {
  parse_config();
  printf("Starting...\n");
  signal(SIGTERM, cleanup);
  //signal(SIGINT, cleanup);

  // Start display thread
  pthread_t display_thread;
  pthread_create(&display_thread, NULL, display_graphic_bar, NULL);

  // Wait for display thread
  pthread_join(display_thread, NULL);

  return 0;
}

void *launchModules(void *) {
  // Start i3 thread
  pthread_t i3_thread;
  pthread_create(&i3_thread, NULL, listen_to_i3, NULL);

  // Start modules threads
  pthread_t threads[modules_count];
  struct Module *current = head;
  int i = 0;
  while (current != NULL) {
    printf("Starting module %s\n", current->name);
    pthread_create(&threads[i], NULL, current->thread_function, current);
    current = current->next;
    i++;
  }
  // Wait for modules threads
  for (int i = 0; i < modules_count; ++i) {
    pthread_join(threads[i], NULL);
  }

  // Wait for i3 thread
  pthread_join(i3_thread, NULL);

  return NULL;
}

void cleanup(int) {
  running = 0;
  // pa_mainloop_quit(volume_loop, 0); // TODO enable this again
  // pa_mainloop_quit(mic_loop, 0);
}

int remove_nl(char *str) {
  for (int i = 0; str[i] != '\0'; i++) {
    if (str[i] == '\n') {
      str[i] = '\0';
      return 0;
    }
  }
  return 1;
}