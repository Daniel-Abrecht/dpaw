#ifndef DPAW_H
#define DPAW_H

#include <dpawindow/root.h>
#include <linked_list.h>

enum {
  DPAW_WORKSPACE_MAX_TOUCH_SOURCES = 256
};

struct dpaw_touchevent_window_map {
  int touchid;
  struct dpawindow* window;
};

struct dpaw {
  struct dpawindow_root root;
  struct dpaw_list window_list;
  struct dpaw_list window_update_list;
  int last_touch;
  struct dpaw_touchevent_window_map touch_source[DPAW_WORKSPACE_MAX_TOUCH_SOURCES];
  int x11_fd;
};

int dpaw_cleanup(struct dpaw*);
int dpaw_init(struct dpaw*);
int dpaw_run(struct dpaw*);
int dpaw_error_handler(Display*, XErrorEvent*);

#endif
