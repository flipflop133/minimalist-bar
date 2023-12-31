#ifndef DEFS_H
#define DEFS_H
#include <pthread.h>

int remove_nl(char *str);
void cleanup(int sig);
extern void *launchModules(void *);

char *retrieve_command_arg(char *arg);
// Command line arguments are stored here
extern struct Argument *argument_head;
extern int args_count;
struct Argument
{
  struct Argument *next;
  char *name;
  void *value;
};

// Modules positions
enum positions
{
  RIGHT,
  CENTER,
  LEFT
};

// Basic structure of a module
struct Module
{
  struct Module *next;
  char name[50];
  void *(*thread_function)(void *);
  char *string;
  int position;
  void *Module_infos;
};

extern struct Module *head;
extern int modules_count;
// Structures for specific modules
struct Network
{
  char *interface;
};

struct Battery
{
  char *battery; // default battery
};

struct Date
{
  char *format;
};

struct Media
{
  int title_max_length;
  int artist_max_length;
};

typedef struct
{
  int max_length_title;
  int max_length_artist;
} Media;

/*Bar options*/
typedef struct
{
  // Colors
  char background_color[8];
  char foreground_color[8];
  char workspace_color[8];
  char workspace_foreground_color[8];
  char workspace_color_urgent[8];

  // Font
  char *font_name;
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

enum modules_names
{
  date,
  network,
  bluetooth,
  volume,
  mic,
  media,
  battery
};

extern int running; // Global state of the application

// Global mutex used to avoid modules race conditions
extern pthread_mutex_t mutex;

// Display order of modules
typedef struct
{
  int *list;
  int size;
} DisplayOrder;
extern DisplayOrder displayOrder;

#endif