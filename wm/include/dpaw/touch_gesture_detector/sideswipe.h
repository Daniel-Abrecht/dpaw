#ifndef SIDESWIPE_H
#define SIDESWIPE_H

#include <dpaw/touch_gesture_manager.h>

struct dpaw_sideswipe_detector_params {
  unsigned mask;
  struct dpaw_rect* bounds;
};

struct dpaw_sideswipe_detector {
  struct dpaw_touch_gesture_detector detector;
  struct dpaw_sideswipe_detector_params params;
  const struct dpaw* dpaw;
  int touch_source;
  enum dpaw_direction direction;
  struct dpaw_point initial_position;
};

int dpaw_sideswipe_init(
  struct dpaw_sideswipe_detector* sideswipe,
  const struct dpaw* dpaw,
  const struct dpaw_sideswipe_detector_params* params
);

#endif
