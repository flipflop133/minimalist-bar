#include "../defs.h"
#include "../i3ipc.h"
#include "display.h"

/**
 * Displays the workspaces.
 */
void display_workspaces() {
  for (int screen_num = 0; screen_num < display_config->num_screens;
       screen_num++) {
    int workspace_padding = options.workspace_padding;
    int padding = 20;
    int xCoordinate = workspace_padding / 2;
    XGlyphInfo extents;

    // Clear workspaces area
    if (screen_configs[screen_num].workspaces_init) {
      XClearArea(display_config->display, screen_configs[screen_num].window, 0,
                 0, screen_configs[screen_num].workspaces_width,
                 display_config->screen_info->height, False);
    }

    // Display workspaces
    for (unsigned short i = 0; i < workspaces.size; i++) {
      if (workspaces.workspaces[i].x ==
          display_config->screen_info[screen_num].x_org) {

        if (workspaces.workspaces[i].num == 0) {
          break;
        }
        char str[workspaces.workspaces[i].num];

        sprintf(str, "%d", workspaces.workspaces[i].num);
        XftTextExtentsUtf8(display_config->display, display_config->font,
                           (XftChar8 *)str, strlen(str), &extents);

        XftColor text_color = display_config->xftColor;
        if (workspaces.workspaces[i].urgent ||
            workspaces.workspaces[i].visible) {
          char *color = workspaces.workspaces[i].urgent
                            ? options.workspace_color_urgent
                            : options.workspace_color;
          XSetForeground(display_config->display, display_config->gc,
                         hex_color_to_pixel(color, 0));
          XFillRectangle(display_config->display,
                         screen_configs[screen_num].window, display_config->gc,
                         xCoordinate - workspace_padding / 2, 0,
                         extents.xOff + workspace_padding, options.bar_height);
          text_color = display_config->workspace_foreground_color;
        }

        XftDrawStringUtf8(screen_configs[screen_num].xftDraw, &text_color,
                          display_config->font, xCoordinate,
                          display_config->yFontCoordinate,
                          (const unsigned char *)str, strlen(str));
        xCoordinate = xCoordinate + extents.xOff + padding;
      }
      if (resize_mode) {
        XftTextExtentsUtf8(display_config->display, display_config->font,
                           (XftChar8 *)RESIZE_MODE_STRING,
                           strlen(RESIZE_MODE_STRING), &extents);

        XftDrawStringUtf8(screen_configs[screen_num].xftDraw,
                          &display_config->xftColor, display_config->font,
                          xCoordinate, display_config->yFontCoordinate,
                          (const unsigned char *)RESIZE_MODE_STRING,
                          strlen(RESIZE_MODE_STRING));
        xCoordinate = xCoordinate + extents.xOff + padding;
      }
      screen_configs[screen_num].workspaces_width = xCoordinate;
      XFlush(display_config->display);
      screen_configs[screen_num].workspaces_init = 1;
    }
  }
}