#ifndef PRIMITIVES_H
#define PRIMITIVES_H

struct dpaw_point {
  long x, y;
};

struct dpaw_rect {
  struct dpaw_point top_left;
  struct dpaw_point bottom_right;
};

enum dpaw_direction {
  DPAW_DIRECTION_RIGHTWARDS,
  DPAW_DIRECTION_DOWNWARDS,
  DPAW_DIRECTION_LEFTWARDS,
  DPAW_DIRECTION_UPWARDS
};

#endif
