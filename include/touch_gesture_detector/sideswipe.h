#ifndef SIDESWIPE_H
#define SIDESWIPE_H

#include <touch_gesture_manager.h>

struct dpaw_sideswipe_detector_params {
  unsigned mask;
  unsigned switch_distance;
  void* private;
  void (*onswipe)(void* private, enum dpaw_direction direction, long count, long y);
};

struct dpaw_sideswipe_detector {
  struct dpaw_touch_gesture_detector detector;
  struct dpaw_sideswipe_detector_params params;
  const struct dpaw* dpaw;
  int touchid;
  bool confirmed;
  bool match;
  long last;
  enum dpaw_direction direction;
  struct dpaw_point initial_position;
};

int dpaw_sideswipe_init(
  struct dpaw_sideswipe_detector* sideswipe,
  const struct dpaw_sideswipe_detector_params* params,
  const struct dpaw* dpaw
);

#endif
