#include "display.h"
#include "defs.h"
#include "i3ipc.h"
#include <X11/Xatom.h>
#include <X11/Xdefs.h>
#include <X11/Xft/Xft.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xinerama.h>
#include <X11/extensions/Xrandr.h>
#include <cairo/cairo-ft.h>
#include <cairo/cairo-xlib.h>
#include <cairo/cairo.h>
#include <ft2build.h>
#include <i3/ipc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include FT_FREETYPE_H

#define BAR_NAME "minimalist_bar"
#define RESIZE_MODE_STRING "resize"

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
};

Display *display;
GC gc;
Window windows[128];
XftFont *font;

XftColor xftColor;
int num_screens;
XftColor workspace_foreground_color;
XineramaScreenInfo *screen_info;
struct ScreenConfig screen_configs[128]; // TODO : use a dynamic array
int yFontCoordinate;
void drawModuleString(int x, int y, char *string, int screen_num);
void clearModuleArea(int x, int y, int width, int screen_num);
unsigned long hex_color_to_pixel(char *hex_color, int screen_num)
{
  XColor color;
  Colormap colormap = DefaultColormap(display, screen_num);
  XParseColor(display, colormap, hex_color, &color);
  XAllocColor(display, colormap, &color);
  return color.pixel;
}

void *display_graphic_bar(void *)
{
  display = XOpenDisplay(NULL);
  if (display == NULL)
  {
    fprintf(stderr, "Unable to open X display\n");
    exit(1);
  }

  screen_info = XineramaQueryScreens(display, &num_screens);

  unsigned long background_color =
      hex_color_to_pixel(options.background_color, DefaultScreen(display));

  Window root_window = RootWindow(display, DefaultScreen(display));

  for (int i = 0; i < num_screens; i++) {
    screen_configs[i].right_padding = 10;
    screen_configs[i].modules_right_x = screen_info[i].x_org;
    screen_configs[i].modules_center_x = screen_info[i].x_org;
    screen_configs[i].modules_center_width = 0;
    screen_configs[i].modules_left_x = screen_info[i].x_org;
    screen_configs[i].modules_left_width = 0;
    screen_configs[i].workspaces_width = 0;
    screen_configs[i].workspaces_init = 0;

    int y_position = strcmp(options.bar_position, "top")
                         ? (screen_info[i].height - options.bar_height)
                         : 0;
    windows[i] = XCreateSimpleWindow(
        display, root_window, screen_info[i].x_org, y_position,
        screen_info[i].width, options.bar_height, 0, 0, background_color);
    XStoreName(display, windows[i], BAR_NAME);
    XSelectInput(display, windows[i], ExposureMask | KeyPressMask);

    Atom net_wm_window_property =
        XInternAtom(display, "_NET_WM_WINDOW_TYPE", False);
    Atom net_wm_window_type_dock =
        XInternAtom(display, "_NET_WM_WINDOW_TYPE_DOCK", False);

    XChangeProperty(display, windows[i], net_wm_window_property, XA_ATOM, 32,
                    PropModeReplace, (unsigned char *)&net_wm_window_type_dock,
                    1);

    XMapWindow(display, windows[i]);

    gc = XCreateGC(display, windows[i], 0, NULL);

    font = XftFontOpen(display, DefaultScreen(display), XFT_FAMILY,
                       XftTypeString, options.font_name, XFT_SIZE,
                       XftTypeDouble, (float)options.font_size, (void *)0);

    screen_configs[i].xftDraw = XftDrawCreate(
        display, windows[i], DefaultVisual(display, DefaultScreen(display)),
        DefaultColormap(display, DefaultScreen(display)));
    XftColorAllocName(display, DefaultVisual(display, DefaultScreen(display)),
                      DefaultColormap(display, DefaultScreen(display)),
                      options.foreground_color, &xftColor);
    XftColorAllocName(display, DefaultVisual(display, DefaultScreen(display)),
                      DefaultColormap(display, DefaultScreen(display)),
                      options.workspace_foreground_color,
                      &workspace_foreground_color);
  }
  // Enable resolution changes detection
  XRRSelectInput(display, root_window, RRScreenChangeNotifyMask);
  yFontCoordinate = (options.bar_height + font->ascent - font->descent) / 2;
  // Start modules thread
  pthread_t modules_thread;
  pthread_create(&modules_thread, NULL, launchModules, NULL);

  XEvent event;
  int res_changed;
  while (1) {
    res_changed = 0;
    XNextEvent(display, &event);
    // Update bar display if resolution changes
    if (XRRUpdateConfiguration(&event)) {
      printf("screen resolution changed.\n");
      res_changed = 1;
    }
    if (event.type == Expose) {
      display_modules(LEFT);
      display_modules(CENTER);
      display_modules(RIGHT);
      if (!res_changed) {
        display_workspaces(); // TODO don't update here if resolution change
                              // as it crashes!
      } else {
        // tell i3ipc we need an update!
      }
    }
  }
  // Wait for i3 thread
  pthread_join(modules_thread, NULL);
  XftColorFree(display, DefaultVisual(display, DefaultScreen(display)),
               DefaultColormap(display, DefaultScreen(display)), &xftColor);
  XftColorFree(display, DefaultVisual(display, DefaultScreen(display)),
               DefaultColormap(display, DefaultScreen(display)), &workspace_foreground_color);
  XftFontClose(display, font);
  XftDrawDestroy(screen_configs[0].xftDraw);
  XDestroyWindow(display, windows[0]);
  XCloseDisplay(display);
  return 0;
}

void display_workspaces()
{
  for (int screen_num = 0; screen_num < num_screens; screen_num++) {

    int workspace_padding = options.workspace_padding;
    int padding = 20;
    int xCoordinate = workspace_padding / 2;
    XGlyphInfo extents;
    if (screen_configs[screen_num].workspaces_init) {
      XClearArea(display, windows[screen_num], 0, 0,
                 screen_configs[screen_num].workspaces_width,
                 screen_info[screen_num].height, False);
    }

    for (unsigned short i = 0; i < workspaces.size; i++) {
      if (workspaces.workspaces[i].x == screen_info[screen_num].x_org) {
        if (workspaces.workspaces[i].num == 0) {
          break;
        }
        char str[workspaces.workspaces[i].num];

        sprintf(str, "%d", workspaces.workspaces[i].num);
        XftTextExtentsUtf8(display, font, (XftChar8 *)str, strlen(str),
                           &extents);

        XftColor text_color = xftColor;
        if (workspaces.workspaces[i].urgent ||
            workspaces.workspaces[i].visible) {
          char *color = workspaces.workspaces[i].urgent
                            ? options.workspace_color_urgent
                            : options.workspace_color;
          XSetForeground(display, gc, hex_color_to_pixel(color, 0));
          XFillRectangle(display, windows[screen_num], gc,
                         xCoordinate - workspace_padding / 2, 0,
                         extents.xOff + workspace_padding, options.bar_height);
          text_color = workspace_foreground_color;
        }

        XftDrawStringUtf8(screen_configs[screen_num].xftDraw, &text_color, font,
                          xCoordinate, yFontCoordinate,
                          (const unsigned char *)str, strlen(str));
        xCoordinate = xCoordinate + extents.xOff + padding;
      }
      if (resize_mode) {
        XftTextExtentsUtf8(display, font, (XftChar8 *)RESIZE_MODE_STRING,
                           strlen(RESIZE_MODE_STRING), &extents);

        XftDrawStringUtf8(screen_configs[screen_num].xftDraw, &xftColor, font,
                          xCoordinate, yFontCoordinate,
                          (const unsigned char *)RESIZE_MODE_STRING,
                          strlen(RESIZE_MODE_STRING));
        xCoordinate = xCoordinate + extents.xOff + padding;
      }
      screen_configs[screen_num].workspaces_width = xCoordinate;
      XFlush(display);
      screen_configs[screen_num].workspaces_init = 1;
    }
  }
}

void display_modules(int position)
{
  for (int screen_num = 0; screen_num < num_screens; screen_num++) {
    pthread_mutex_lock(&mutex);
    if (display == NULL || font == NULL ||
        screen_configs[screen_num].xftDraw == NULL) {
      return;
    }

    int xCoordinate_left = screen_configs[screen_num].modules_left_x;
    int xCoordinate_center = screen_configs[screen_num].modules_center_x;
    int xCoordinate_right = screen_configs[screen_num].modules_right_x;
    // Clear modules area
    switch (position) {
    case LEFT:
      clearModuleArea(screen_configs[screen_num].modules_left_x, 0,
                      screen_configs[screen_num].modules_left_width,
                      screen_num);
      xCoordinate_left = screen_configs[screen_num].workspaces_width;
      break;
    case CENTER:
      clearModuleArea(screen_configs[screen_num].modules_center_x, 0,
                      screen_configs[screen_num].modules_center_width,
                      screen_num);
      screen_configs[screen_num].modules_center_width = 0;
      xCoordinate_center = screen_info[screen_num].width / 2;
      break;
    case RIGHT:
      clearModuleArea(screen_configs[screen_num].modules_right_x, 0,
                      screen_info[screen_num].width, screen_num);
      xCoordinate_right = (screen_info[screen_num].width -
                           screen_configs[screen_num].right_padding);
      break;
    default:
      break;
    }
    // Display modules
    XGlyphInfo extents;
    struct Module *current = head;
    while (current != NULL) {
      if (current->position == position) {
        XftTextExtentsUtf8(display, font, (XftChar8 *)current->string,
                           strlen(current->string), &extents);
        switch (current->position) {
        case LEFT:
          xCoordinate_left = xCoordinate_left + options.module_left_padding;
          drawModuleString(xCoordinate_left, yFontCoordinate, current->string,
                           screen_num);
          break;
        case CENTER:
          xCoordinate_center -= extents.xOff / 2;
          drawModuleString(xCoordinate_center, yFontCoordinate, current->string,
                           screen_num);
          screen_configs[screen_num].modules_center_width += extents.xOff;
          break;
        case RIGHT:
          xCoordinate_right -= extents.xOff;
          drawModuleString(xCoordinate_right, yFontCoordinate, current->string,
                           screen_num);
          xCoordinate_right -= options.module_left_padding;
          break;
        default:
          break;
        }
      }
      current = current->next;
    }
    screen_configs[screen_num].modules_right_x = xCoordinate_right;
    screen_configs[screen_num].modules_left_x = xCoordinate_left;
    screen_configs[screen_num].modules_center_x = xCoordinate_center;
    XFlush(display);
    pthread_mutex_unlock(&mutex);
  }
}

void clearModuleArea(int x, int y, int width, int screen_num) {
  if (width != 0)
  {
    XClearArea(display, windows[screen_num], x, y, width,
               screen_info[screen_num].height, False);
  }
}

void drawModuleString(int x, int y, char *string, int screen_num) {
  XftDrawStringUtf8(screen_configs[screen_num].xftDraw, &xftColor, font, x, y,
                    (const FcChar8 *)string, strlen(string));
}
