#ifndef DEFS_H
#define DEFS_H
#define IW_INTERFACE "wlan0"
#include <pthread.h>
enum positions { RIGHT, CENTER, LEFT };
typedef struct {
  const char *name;
  void *(*thread_function)(void *);
  int enabled;
  char *string;
  const int interval;
  const int position;
} ModuleInfo;

extern ModuleInfo modules[];
enum modules_names { date, network, bluetooth, volume, mic, media, battery};
enum pulse_events { SINK, SOURCE };
int remove_nl(char *str);
void cleanup(int sig);
extern int running;
extern pthread_mutex_t mutex;
typedef struct {
  int *list;
  int size;
} DisplayOrder;
extern DisplayOrder displayOrder;
extern void* launchModules(void*);
#endif