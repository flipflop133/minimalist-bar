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

typedef struct
{
  // Colors
  char background_color[8];
  char foreground_color[8];
  char workspace_color[8];
  char workspace_color_urgent[8];

  // Font
  char* font_name;
  int font_size;

  // Sizes
  int workspace_padding;
  int right_padding;
  int module_left_padding;
  int bar_height;

  // Other
  char bar_position[7];
} Options;
extern Options options;


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