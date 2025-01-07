
#ifndef DISPLAY_H
#define DISPLAY_H

#include <X11/Xft/Xft.h>
#include <X11/extensions/Xinerama.h>

// Function declarations
extern void *display_graphic_bar(void *);
extern void display_modules(int position);
extern void display_workspaces(void);
extern void drawModuleString(int x, int y, char *string, int screen_num);
extern void clearModuleArea(int x, int y, int width, int screen_num);
unsigned long hex_color_to_pixel(char *hex_color, int screen_num);

// Macro definitions
#define BAR_NAME "minimalist_bar"
#define RESIZE_MODE_STRING "resize"

// Structure definitions
struct ScreenConfig {
  int right_padding;
  int modules_right_x;
  int modules_center_x;
  int modules_center_width;
  int modules_left_x;
  int modules_left_width;
  XftDraw *xftDraw;
  int workspaces_width;
  // Needed because if we use XClearArea with workspaces_width = 0, we clear the
  // whole bar including the modules!
  int workspaces_init;
  Window window;
};

struct DisplayConfig {
  Display *display;
  GC gc;
  XftFont *font;
  XftColor xftColor;
  int num_screens;
  XftColor workspace_foreground_color;
  int yFontCoordinate;
  XineramaScreenInfo *screen_info;
};

// Variable declarations
extern struct ScreenConfig screen_configs[128]; // TODO : use a dynamic array
extern struct DisplayConfig *display_config;

#endif