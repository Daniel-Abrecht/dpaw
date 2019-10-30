#ifndef DPAWIN_H
#define DPAWIN_H

#include <dpawindow/root.h>

enum {
  DPAWIN_WORKSPACE_MAX_TOUCH_SOURCES = 256
};

struct dpawin_touchevent_window_map {
  int touchid;
  struct dpawindow* window;
};

struct dpawin {
  struct dpawindow_root root;
  struct dpawindow *first, *last;
  int last_touch;
  struct dpawin_touchevent_window_map touch_source[DPAWIN_WORKSPACE_MAX_TOUCH_SOURCES];
};

int dpawin_cleanup(struct dpawin*);
int dpawin_init(struct dpawin*);
int dpawin_run(struct dpawin*);
int dpawin_error_handler(Display*, XErrorEvent*);

#endif
