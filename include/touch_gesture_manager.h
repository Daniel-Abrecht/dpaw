#ifndef TOUCH_GESTURE_DETECTOR_H
#define TOUCH_GESTURE_DETECTOR_H

#include <linked_list.h>
#include <primitives.h>
#include <xev/xinput2.c>

struct dpawin_touch_gesture_manager {
  struct dpawin_list detector_list;
  struct dpawin_touch_gesture_detector* detected;
};

struct dpawin_touch_gesture_detector;
struct dpawin_touch_gesture_detector_type {
  enum event_handler_result (*ontouch)(
    struct dpawin_touch_gesture_detector* detector,
    XIDeviceEvent* event,
    struct dpawin_rect bounds
  );
  void (*cleanup)(struct dpawin_touch_gesture_detector* detector);
  void (*reset)(struct dpawin_touch_gesture_detector* detector);
};

struct dpawin_touch_gesture_detector {
  const struct dpawin_touch_gesture_detector_type* type;
  struct dpawin_list_entry manager_entry;
};

int dpawin_touch_gesture_manager_init(struct dpawin_touch_gesture_manager*);
void dpawin_touch_gesture_manager_reset(struct dpawin_touch_gesture_manager*);
void dpawin_touch_gesture_manager_cleanup(struct dpawin_touch_gesture_manager*);

int dpawin_touch_gesture_manager_add_detector(
  struct dpawin_touch_gesture_manager*,
  struct dpawin_touch_gesture_detector*
);

enum event_handler_result dpawin_touch_gesture_manager_dispatch_touch(
  struct dpawin_touch_gesture_manager*,
  XIDeviceEvent*,
  struct dpawin_rect
);

#endif
