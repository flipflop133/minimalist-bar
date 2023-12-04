#include "display.h"
#include "defs.h"
#include "i3ipc.h"
#include <X11/Xdefs.h>
#include <X11/Xft/Xft.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
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
#include <X11/extensions/Xrandr.h>
#include FT_FREETYPE_H

#define BAR_NAME "minimalist_bar"
#define RESIZE_MODE_STRING "resize"
Display *display;
int screen_num;
GC gc;
Window window;
XftFont *font;
XftDraw *xftDraw;
XftColor xftColor;
XftColor workspace_foreground_color;
int yFontCoordinate;
void drawModuleString(int x, int y, char *string);
void clearModuleArea(int x, int y, int width);
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
  screen_num = DefaultScreen(display);
  if (display == NULL)
  {
    fprintf(stderr, "Unable to open X display\n");
    exit(1);
  }

  unsigned long background_color =
      hex_color_to_pixel(options.background_color, screen_num);

  Window root_window = RootWindow(display, screen_num);

  int y_position =
      strcmp(options.bar_position, "top")
          ? (DisplayHeight(display, screen_num) - options.bar_height)
          : 0;

  window = XCreateSimpleWindow(display, root_window, 0, y_position,
                               DisplayWidth(display, screen_num),
                               options.bar_height, 0, 0, background_color);
  XStoreName(display, window, BAR_NAME);
  XSelectInput(display, window, ExposureMask | KeyPressMask);

  Atom net_wm_window_property =
      XInternAtom(display, "_NET_WM_WINDOW_TYPE", False);
  Atom net_wm_window_type = XInternAtom(display, "WM_HINTS", False);
  Atom net_wm_window_type_dock =
      XInternAtom(display, "_NET_WM_WINDOW_TYPE_DOCK", False);

  XChangeProperty(display, window, net_wm_window_property, net_wm_window_type,
                  32, PropModeReplace,
                  (unsigned char *)&net_wm_window_type_dock, 1);

  XMapWindow(display, window);

  gc = XCreateGC(display, window, 0, NULL);

  font = XftFontOpen(display, DefaultScreen(display), XFT_FAMILY, XftTypeString,
                     options.font_name, XFT_SIZE, XftTypeDouble,
                     (float)options.font_size, (void *)0);

  xftDraw = XftDrawCreate(display, window,
                          DefaultVisual(display, DefaultScreen(display)),
                          DefaultColormap(display, DefaultScreen(display)));
  XftColorAllocName(display, DefaultVisual(display, DefaultScreen(display)),
                    DefaultColormap(display, DefaultScreen(display)),
                    options.foreground_color, &xftColor);
  XftColorAllocName(display, DefaultVisual(display, DefaultScreen(display)),
                    DefaultColormap(display, DefaultScreen(display)),
                    options.workspace_foreground_color, &workspace_foreground_color);
  yFontCoordinate = (options.bar_height + font->ascent - font->descent) / 2;
  // Start modules thread
  pthread_t modules_thread;
  pthread_create(&modules_thread, NULL, launchModules, NULL);

  // Enable resolution changes detection
  XRRSelectInput(display, RootWindow(display, screen_num), RRScreenChangeNotifyMask);

  XEvent event;
  int res_changed;
  while (1)
  {
    res_changed = 0;
    XNextEvent(display, &event);
    // Update bar display if resolution changes
    if (XRRUpdateConfiguration(&event))
    {
      printf("screen resolution changed.\n");
      res_changed = 1;
    }
    if (event.type == Expose)
    {
      display_modules(LEFT);
      display_modules(CENTER);
      display_modules(RIGHT);
      if (!res_changed)
      {
        display_workspaces(); // TODO don't update here if resolution change as it crashes!
      }
      else
      {
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
  XftDrawDestroy(xftDraw);
  XDestroyWindow(display, window);
  XCloseDisplay(display);
  return 0;
}

int workspaces_width = 0;
// Needed because if we use XClearArea with workspaces_width = 0, we clear the
// whole bar including the modules!
int workspaces_init = 0;
void display_workspaces()
{
  int workspace_padding = options.workspace_padding;
  int padding = 20;
  int xCoordinate = workspace_padding / 2;
  XGlyphInfo extents;
  if (workspaces_init)
  {
    XClearArea(display, window, 0, 0, workspaces_width,
               DisplayHeight(display, screen_num), False);
  }

  for (unsigned short i = 0; i < workspaces.size; i++)
  {
    if (workspaces.workspaces[i].num == 0)
    {
      break;
    }
    char str[workspaces.workspaces[i].num];

    sprintf(str, "%d", workspaces.workspaces[i].num);
    XftTextExtentsUtf8(display, font, (XftChar8 *)str, strlen(str), &extents);

    XftColor text_color = xftColor;
    if (workspaces.workspaces[i].urgent || workspaces.workspaces[i].visible)
    {
      char *color = workspaces.workspaces[i].urgent
                        ? options.workspace_color_urgent
                        : options.workspace_color;
      XSetForeground(display, gc, hex_color_to_pixel(color, screen_num));
      XFillRectangle(display, window, gc, xCoordinate - workspace_padding / 2,
                     0, extents.xOff + workspace_padding, options.bar_height);
      text_color = workspace_foreground_color;
    }
    XftDrawStringUtf8(xftDraw, &text_color, font, xCoordinate, yFontCoordinate,
                      (const unsigned char *)str, strlen(str));
    xCoordinate = xCoordinate + extents.xOff + padding;
  }
  if (resize_mode)
  {
    XftTextExtentsUtf8(display, font, (XftChar8 *)RESIZE_MODE_STRING, strlen(RESIZE_MODE_STRING), &extents);

    XftDrawStringUtf8(xftDraw, &xftColor, font, xCoordinate, yFontCoordinate,
                      (const unsigned char *)RESIZE_MODE_STRING, strlen(RESIZE_MODE_STRING));
    xCoordinate = xCoordinate + extents.xOff + padding;
  }
  workspaces_width = xCoordinate;
  XFlush(display);
  workspaces_init = 1;
}

int right_padding = 10;
int modules_right_x = 0;
int modules_center_x = 0;
int modules_center_width = 0;
int modules_left_x = 0;
int modules_left_width = 0;
void display_modules(int position)
{
  pthread_mutex_lock(&mutex);
  if (display == NULL || font == NULL || xftDraw == NULL)
  {
    return;
  }

  int xCoordinate_left = modules_left_x;
  int xCoordinate_center = modules_center_x;
  int xCoordinate_right = modules_right_x;
  // Clear modules area
  switch (position)
  {
  case LEFT:
    clearModuleArea(modules_left_x, 0, modules_left_width);
    xCoordinate_left = workspaces_width;
    break;
  case CENTER:
    clearModuleArea(modules_center_x, 0, modules_center_width);
    modules_center_width = 0;
    xCoordinate_center = DisplayWidth(display, screen_num) / 2;
    break;
  case RIGHT:
    clearModuleArea(modules_right_x, 0, DisplayWidth(display, screen_num));
    xCoordinate_right = DisplayWidth(display, screen_num) - right_padding;
    break;
  default:
    break;
  }

  // Display modules
  XGlyphInfo extents;
  struct Module *current = head;
  while (current != NULL)
  {
    if (current->position == position)
    {
      XftTextExtentsUtf8(display, font, (XftChar8 *)current->string,
                         strlen(current->string), &extents);
      switch (current->position)
      {
      case LEFT:
        xCoordinate_left = xCoordinate_left + options.module_left_padding;
        drawModuleString(xCoordinate_left, yFontCoordinate, current->string);
        break;
      case CENTER:
        xCoordinate_center -= extents.xOff / 2;
        drawModuleString(xCoordinate_center, yFontCoordinate, current->string);
        modules_center_width += extents.xOff;
        break;
      case RIGHT:
        xCoordinate_right -= extents.xOff;
        drawModuleString(xCoordinate_right, yFontCoordinate, current->string);
        xCoordinate_right -= options.module_left_padding;
        break;
      default:
        break;
      }
    }
    current = current->next;
  }
  modules_right_x = xCoordinate_right;
  modules_left_x = xCoordinate_left;
  modules_center_x = xCoordinate_center;
  XFlush(display);
  pthread_mutex_unlock(&mutex);
}

void clearModuleArea(int x, int y, int width)
{
  if (width != 0)
  {
    XClearArea(display, window, x, y, width, DisplayHeight(display, screen_num),
               False);
  }
}

void drawModuleString(int x, int y, char *string)
{
  XftDrawStringUtf8(xftDraw, &xftColor, font, x, y, (const FcChar8 *)string,
                    strlen(string));
}
