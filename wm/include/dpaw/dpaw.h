#ifndef DPAW_H
#define DPAW_H

#include <dpaw/linked_list.h>
#include <dpaw/dpawindow/root.h>

enum {
  DPAW_WORKSPACE_MAX_TOUCH_SOURCES = 128
};

struct dpaw_touchevent_window_map {
  int touchid;
  struct dpawindow* window;
  struct dpaw_touch_gesture_detector* gesture_detector;
};

struct dpaw {
  struct dpawindow_root root;
  struct dpaw_list window_list;
  struct dpaw_list window_update_list;
  struct dpaw_touchevent_window_map touch_source[DPAW_WORKSPACE_MAX_TOUCH_SOURCES];
  int last_touch;
  int x11_fd;
  bool initialised;
};

int dpaw_cleanup(struct dpaw*);
int dpaw_init(struct dpaw*);
int dpaw_run(struct dpaw*);
int dpaw_error_handler(Display*, XErrorEvent*);

#endif
