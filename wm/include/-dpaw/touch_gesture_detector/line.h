#ifndef TOUCHGESTURE_LINE_TOUCH_H
#define TOUCHGESTURE_LINE_TOUCH_H

#include <-dpaw/primitives.h>
#include <-dpaw/touch_gesture_manager.h>

struct dpaw_line_touch_detector_params {
  struct dpaw_line line;
  bool noclip;
};

struct dpaw_line_touch_detector {
  struct dpaw_touch_gesture_detector detector;
  struct dpaw_line_touch_detector_params params;
  const struct dpaw* dpaw;
};

int dpaw_line_touch_init(
  struct dpaw_line_touch_detector* sideswipe,
  const struct dpaw_line_touch_detector_params* params,
  const struct dpaw* dpaw
);

#endif
