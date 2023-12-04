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

void parse_arguments(int argc, char *argv[]);
DisplayOrder displayOrder;
Options options;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
struct Module *head = NULL;
int modules_count = 0;
int running = 1;
struct Argument *argument_head;
int args_count;

int main(int argc, char *argv[])
{
  parse_arguments(argc, argv);
  parse_config();
  signal(SIGTERM, cleanup);
  signal(SIGINT, cleanup);

  // Start display thread
  pthread_t display_thread;
  pthread_create(&display_thread, NULL, display_graphic_bar, NULL);

  // Wait for display thread
  pthread_join(display_thread, NULL);

  return 0;
}

char *retrieve_command_arg(char *arg)
{
  struct Argument *current = argument_head;
  while (current != NULL)
  {
    if (strcmp(current->name, arg) == 0)
    {
      return current->value;
    }
    current = current->next;
  }
  return NULL;
}

void parse_arguments(int argc, char *argv[])
{
  args_count = argc;
  struct Argument *current = NULL;
  struct Argument *previous = NULL;
  int first = 1;
  for (int i = 0; i < argc; i++)
  {
    current = (struct Argument *)malloc(sizeof(struct Argument));
    current->name = (char *)malloc(sizeof(char) * strlen(argv[i]));
    strcpy(current->name, argv[i]);
    if (strcmp(argv[i], "--config") == 0)
    {
      current->value = (char *)malloc(sizeof(char) * strlen(argv[++i]));
      strcpy(current->value, argv[i]);
    }
    if (first)
    {
      first = 0;
      argument_head = current;
    }
    else
    {
      previous->next = current;
    }
    previous = current;
  }
}

void *launchModules(void *)
{
  // Start i3 thread
  pthread_t i3_thread;
  pthread_create(&i3_thread, NULL, listen_to_i3, NULL);

  // Start modules threads
  pthread_t threads[modules_count];
  struct Module *current = head;
  int i = 0;
  while (current != NULL)
  {
    pthread_create(&threads[i], NULL, current->thread_function, current);
    current = current->next;
    i++;
  }

  // Wait for modules threads
  for (int i = 0; i < modules_count; ++i)
  {
    pthread_join(threads[i], NULL);
  }

  // Wait for i3 thread
  pthread_join(i3_thread, NULL);

  return NULL;
}

void cleanup(int)
{
  running = 0;
  pa_mainloop_quit(volume_loop, 0);
  pa_mainloop_quit(mic_loop, 0);
}

int remove_nl(char *str)
{
  for (int i = 0; str[i] != '\0'; i++)
  {
    if (str[i] == '\n')
    {
      str[i] = '\0';
      return 0;
    }
  }
  return 1;
}