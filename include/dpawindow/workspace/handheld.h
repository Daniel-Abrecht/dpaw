#ifndef WORKSPACE_HANDHELD_H
#define WORKSPACE_HANDHELD_H

#include <workspace.h>
#include <stdbool.h>

enum dpawin_direction {
  DPAWIN_DIRECTION_LEFT_TO_RIGHT,
  DPAWIN_DIRECTION_TOP_TO_BOTTOM,
  DPAWIN_DIRECTION_RIGHT_TO_LEFT,
  DPAWIN_DIRECTION_BOTTOM_TO_TOP
};

struct dpawin_sideswipe_state {
  unsigned mask;
  int touchid;
  bool confirmed;
  bool match;
  long last;
  enum dpawin_direction direction;
  struct dpawin_point initial_position;
};

DECLARE_DPAWIN_WORKSPACE( handheld,
  struct dpawindow_handheld_window* current;
  struct dpawin_sideswipe_state sideswipe;
)

struct dpawindow_handheld_window {
  struct dpawindow_app* app_window;
  struct dpawindow_workspace_handheld* workspace;
};

#endif
