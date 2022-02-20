#ifndef DPAW_H
#define DPAW_H

#include <-dpaw/action.h>
#include <-dpaw/linked_list.h>
#include <-dpaw/dpawindow/root.h>
#include <-dpaw/string.h>
#include <-dpaw/array.h>
#include <stddef.h>
#include <stdint.h>

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

struct dpaw_xi {
  struct dpaw_list master_list;
  struct dpaw_list keyboard_list;
  struct dpaw_list pointer_list;
};

struct dpaw {
  struct dpawindow_root root;
  struct dpaw_list window_list;
  struct dpaw_list deferred_action_list;
  struct dpaw_list process_list;
  struct dpaw_list plugin_list;
  struct dpaw_touchevent_window_map touch_source[DPAW_WORKSPACE_MAX_TOUCH_SOURCES];
  struct dpaw_xi input_device;
  DPAW_ARRAY(struct dpaw_fd) input_list;
  DPAW_ARRAY(struct pollfd) fd_list;
  int last_touch;
  int x11_fd;
  bool initialised;
};

// Monotonic time, it counts up, but from an undefined point. Use only relative to itself.
// One second is exactly 0x10000
#define DPAW_MONOSECOND 0x10000u
typedef uint32_t dpaw_monotime_t; // This will be enough for about 18 hours
dpaw_monotime_t dpaw_monotime_now(void);
extern dpaw_monotime_t dpaw_last_tick;

int dpaw_cleanup(struct dpaw*);
int dpaw_init(struct dpaw*);
int dpaw_run(struct dpaw*);
int dpaw_poll_add(struct dpaw* dpaw, const struct dpaw_fd* input, int events);
void dpaw_poll_remove(struct dpaw* dpaw, const struct dpaw_fd* input);
int dpaw_error_handler(Display*, XErrorEvent*);

#endif
