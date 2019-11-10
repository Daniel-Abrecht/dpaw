#ifndef SIDESWIPE_H
#define SIDESWIPE_H

#include <touch_gesture_manager.h>

struct dpawin_sideswipe_detector_params {
  unsigned mask;
  void* private;
  void (*onswipe)(void* private, enum dpawin_direction direction, long count);
};

struct dpawin_sideswipe_detector {
  struct dpawin_touch_gesture_detector detector;
  struct dpawin_sideswipe_detector_params params;
  int touchid;
  bool confirmed;
  bool match;
  long last;
  enum dpawin_direction direction;
  struct dpawin_point initial_position;
};

int dpawin_sideswipe_init(
  struct dpawin_sideswipe_detector* sideswipe,
  const struct dpawin_sideswipe_detector_params* params
);

#endif
