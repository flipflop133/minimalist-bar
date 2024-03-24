#include "display.h"
#include "../defs.h"
#include <X11/X.h>
#include <X11/Xatom.h>
#include <X11/Xft/Xft.h>
#include <X11/Xft/XftCompat.h>
#include <X11/Xlib.h>
#include <X11/extensions/Xinerama.h>
#include <X11/extensions/Xrandr.h>
#include <X11/extensions/randr.h>
#include <bits/pthreadtypes.h>
#include <fontconfig/fontconfig.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct DisplayConfig *display_config;
struct ScreenConfig screen_configs[128]; // TODO : use a dynamic array

/**
 * @brief Displays a graphic bar.
 *
 * This function is responsible for displaying a graphic bar on the screen.
 * It takes no input parameters and returns no value.
 *
 * @param arg A pointer to the argument passed to the thread (not used in this
 * function).
 *
 * @return None.
 */
void *display_graphic_bar(void *) {
  display_config = (struct DisplayConfig *)malloc(sizeof(struct DisplayConfig));

  display_config->display = XOpenDisplay(NULL);
  if (display_config->display == NULL) {
    fprintf(stderr, "Unable to open X display\n");
    exit(1);
  }

  display_config->screen_info = XineramaQueryScreens(
      display_config->display, &display_config->num_screens);

  unsigned long background_color = hex_color_to_pixel(
      options.background_color, DefaultScreen(display_config->display));

  Window root_window = RootWindow(display_config->display,
                                  DefaultScreen(display_config->display));

  for (int i = 0; i < display_config->num_screens; i++) {
    screen_configs[i].right_padding = 10;
    screen_configs[i].modules_right_x = display_config->screen_info[i].x_org;
    screen_configs[i].modules_center_x = display_config->screen_info[i].x_org;
    screen_configs[i].modules_center_width = 0;
    screen_configs[i].modules_left_x = display_config->screen_info[i].x_org;
    screen_configs[i].modules_left_width = 0;
    screen_configs[i].workspaces_width = 0;
    screen_configs[i].workspaces_init = 0;

    int y_position =
        strcmp(options.bar_position, "top")
            ? (display_config->screen_info[i].height - options.bar_height)
            : 0;
    screen_configs[i].window =
        XCreateSimpleWindow(display_config->display, root_window,
                            display_config->screen_info[i].x_org, y_position,
                            display_config->screen_info[i].width,
                            options.bar_height, 0, 0, background_color);
    XStoreName(display_config->display, screen_configs[i].window, BAR_NAME);
    XSelectInput(display_config->display, screen_configs[i].window,
                 ExposureMask | KeyPressMask);

    Atom net_wm_window_property =
        XInternAtom(display_config->display, "_NET_WM_WINDOW_TYPE", False);
    Atom net_wm_window_type_dock =
        XInternAtom(display_config->display, "_NET_WM_WINDOW_TYPE_DOCK", False);

    XChangeProperty(display_config->display, screen_configs[i].window,
                    net_wm_window_property, XA_ATOM, 32, PropModeReplace,
                    (unsigned char *)&net_wm_window_type_dock, 1);

    XMapWindow(display_config->display, screen_configs[i].window);

    display_config->gc =
        XCreateGC(display_config->display, screen_configs[i].window, 0, NULL);

    display_config->font = XftFontOpen(
        display_config->display, DefaultScreen(display_config->display),
        XFT_FAMILY, XftTypeString, options.font_name, XFT_SIZE, XftTypeDouble,
        (float)options.font_size, (void *)0);

    screen_configs[i].xftDraw =
        XftDrawCreate(display_config->display, screen_configs[i].window,
                      DefaultVisual(display_config->display,
                                    DefaultScreen(display_config->display)),
                      DefaultColormap(display_config->display,
                                      DefaultScreen(display_config->display)));
    XftColorAllocName(display_config->display,
                      DefaultVisual(display_config->display,
                                    DefaultScreen(display_config->display)),
                      DefaultColormap(display_config->display,
                                      DefaultScreen(display_config->display)),
                      options.foreground_color, &display_config->xftColor);
    XftColorAllocName(display_config->display,
                      DefaultVisual(display_config->display,
                                    DefaultScreen(display_config->display)),
                      DefaultColormap(display_config->display,
                                      DefaultScreen(display_config->display)),
                      options.workspace_foreground_color,
                      &display_config->workspace_foreground_color);
  }
  // Enable resolution changes detection
  XRRSelectInput(display_config->display, root_window,
                 RRScreenChangeNotifyMask);
  display_config->yFontCoordinate =
      (options.bar_height + display_config->font->ascent -
       display_config->font->descent) /
      2;
  // Start modules thread
  pthread_t modules_thread;
  pthread_create(&modules_thread, NULL, launchModules, NULL);

  XEvent event;
  int res_changed;
  while (1) {
    res_changed = 0;
    XNextEvent(display_config->display, &event);
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
  XftColorFree(display_config->display,
               DefaultVisual(display_config->display,
                             DefaultScreen(display_config->display)),
               DefaultColormap(display_config->display,
                               DefaultScreen(display_config->display)),
               &display_config->xftColor);
  XftColorFree(display_config->display,
               DefaultVisual(display_config->display,
                             DefaultScreen(display_config->display)),
               DefaultColormap(display_config->display,
                               DefaultScreen(display_config->display)),
               &display_config->workspace_foreground_color);
  XftFontClose(display_config->display, display_config->font);
  for (int i = 0; i < display_config->num_screens; i++) {
    XftDrawDestroy(screen_configs[i].xftDraw);
    XDestroyWindow(display_config->display, screen_configs[i].window);
  }
  XCloseDisplay(display_config->display);
  return 0;
}

/**
 * Clears the module area on the screen.
 *
 * This function clears a rectangular area on the screen starting from the
 * specified coordinates (x, y) with the specified width. The screen number is
 * also provided as an argument to indicate which screen to clear.
 *
 * @param x The x-coordinate of the top-left corner of the area to clear.
 * @param y The y-coordinate of the top-left corner of the area to clear.
 * @param width The width of the area to clear.
 * @param screen_num The number of the screen to clear.
 */
void clearModuleArea(int x, int y, int width, int screen_num) {
  if (width != 0) {
    XClearArea(display_config->display, screen_configs[screen_num].window, x, y,
               width, display_config->screen_info[screen_num].height, False);
  }
}

/**
 * Draws a string on the display module at the specified coordinates.
 *
 * @param x The x-coordinate of the starting position of the string.
 * @param y The y-coordinate of the starting position of the string.
 * @param string The string to be drawn on the display module.
 * @param screen_num The screen number where the string should be drawn.
 */
void drawModuleString(int x, int y, char *string, int screen_num) {
  XftDrawStringUtf8(screen_configs[screen_num].xftDraw,
                    &display_config->xftColor, display_config->font, x, y,
                    (const FcChar8 *)string, strlen(string));
}

/**
 * Converts a hexadecimal color code to a pixel value.
 *
 * @param hex_color The hexadecimal color code to convert.
 * @param screen_num The screen number to convert the color for.
 * @return The pixel value corresponding to the given color code.
 */
unsigned long hex_color_to_pixel(char *hex_color, int screen_num) {
  XColor color;
  Colormap colormap = DefaultColormap(display_config->display, screen_num);
  XParseColor(display_config->display, colormap, hex_color, &color);
  XAllocColor(display_config->display, colormap, &color);
  return color.pixel;
}
