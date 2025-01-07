#include "../defs.h"
#include "display.h"

/**
 * Displays the modules at the specified position.
 *
 * @param position The position at which to display the modules.
 */
void display_modules(int position) {
  for (int screen_num = 0; screen_num < display_config->num_screens;
       screen_num++) {
    pthread_mutex_lock(&mutex);
    if (display_config->display == NULL || display_config->font == NULL ||
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
      xCoordinate_center = display_config->screen_info[screen_num].width / 2;
      break;
    case RIGHT:
      clearModuleArea(screen_configs[screen_num].modules_right_x, 0,
                      display_config->screen_info[screen_num].width,
                      screen_num);
      xCoordinate_right = (display_config->screen_info[screen_num].width -
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
        XftTextExtentsUtf8(display_config->display, display_config->font,
                           (XftChar8 *)current->string, strlen(current->string),
                           &extents);
        switch (current->position) {
        case LEFT:
          xCoordinate_left = xCoordinate_left + options.module_left_padding;
          drawModuleString(xCoordinate_left, display_config->yFontCoordinate,
                           current->string, screen_num);
          break;
        case CENTER:
          xCoordinate_center -= extents.xOff / 2;
          drawModuleString(xCoordinate_center, display_config->yFontCoordinate,
                           current->string, screen_num);
          screen_configs[screen_num].modules_center_width += extents.xOff;
          break;
        case RIGHT:
          xCoordinate_right -= extents.xOff;
          drawModuleString(xCoordinate_right, display_config->yFontCoordinate,
                           current->string, screen_num);
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
    XFlush(display_config->display);
    pthread_mutex_unlock(&mutex);
  }
}