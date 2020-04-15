#ifndef DPAW_H
#define DPAW_H

#include <dpaw/linked_list.h>
#include <dpaw/dpawindow/root.h>
#include <dpaw/array.h>

enum {
  DPAW_WORKSPACE_MAX_TOUCH_SOURCES = 128
};

struct dpaw_touchevent_window_map {
  int touchid;
  struct dpawindow* window;
  struct dpaw_touch_gesture_detector* gesture_detector;
};

typedef void(*dpaw_fd_callback)(struct dpaw* dpaw, int fd, int events, void* ptr);

struct dpaw_fd {
  int fd;
  dpaw_fd_callback callback;
  void* ptr;
  bool keep;
};

struct dpaw {
  struct dpawindow_root root;
  struct dpaw_list window_list;
  struct dpaw_list window_update_list;
  struct dpaw_list process_list;
  struct dpaw_touchevent_window_map touch_source[DPAW_WORKSPACE_MAX_TOUCH_SOURCES];
  DPAW_ARRAY(struct dpaw_fd) input_list;
  DPAW_ARRAY(struct pollfd) fd_list;
  int last_touch;
  int x11_fd;
  bool initialised;
};

int dpaw_cleanup(struct dpaw*);
int dpaw_init(struct dpaw*);
int dpaw_run(struct dpaw*);
int dpaw_poll_add(struct dpaw* dpaw, const struct dpaw_fd* input, int events);
void dpaw_poll_remove(struct dpaw* dpaw, const struct dpaw_fd* input);
int dpaw_error_handler(Display*, XErrorEvent*);

#endif
