#ifndef TOUCH_GESTURE_DETECTOR_H
#define TOUCH_GESTURE_DETECTOR_H

#include <linked_list.h>
#include <primitives.h>
#include <xev/xinput2.c>

struct dpaw_touch_gesture_manager {
  struct dpaw* dpaw;
  struct dpaw_list detector_list;
  struct dpaw_touch_gesture_detector* detected;
};

struct dpaw_touch_gesture_detector;
struct dpaw_touch_gesture_detector_type {
  enum event_handler_result (*ontouch)(
    struct dpaw_touch_gesture_detector* detector,
    struct dpaw_touch_event* event
  );
  void (*cleanup)(struct dpaw_touch_gesture_detector* detector);
  void (*reset)(struct dpaw_touch_gesture_detector* detector);
};

struct dpaw_touch_gesture_detector {
  const struct dpaw_touch_gesture_detector_type* type;
  struct dpaw_list_entry manager_entry;
  void* private;
  void (*ongesture)(void* private, struct dpaw_touch_gesture_detector* detector);
  enum event_handler_result (*ontouch)(
    void* private,
    struct dpaw_touch_gesture_detector* detector,
    struct dpaw_touch_event* event
  );
};

int dpaw_touch_gesture_manager_init(struct dpaw_touch_gesture_manager*, struct dpaw* dpaw);
void dpaw_touch_gesture_manager_reset(struct dpaw_touch_gesture_manager*);
void dpaw_touch_gesture_manager_cleanup(struct dpaw_touch_gesture_manager*);
void dpaw_touch_gesture_manager_remove_detector(struct dpaw_touch_gesture_detector* detector);
void dpaw_gesture_detected(struct dpaw_touch_gesture_detector* detector, unsigned touch_source_count, int touch_source_list[touch_source_count]);

int dpaw_touch_gesture_manager_add_detector(
  struct dpaw_touch_gesture_manager*,
  struct dpaw_touch_gesture_detector*
);

enum event_handler_result dpaw_touch_gesture_manager_dispatch_touch(
  struct dpaw_touch_gesture_manager*,
  struct dpaw_touch_event*
);

#endif
