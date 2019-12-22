#ifndef PRIMITIVES_H
#define PRIMITIVES_H

#include <stdbool.h>

#define DPAW_DEFAULT_PPMM 3.779527559

struct dpaw_point {
  long x, y;
};

struct dpaw_rect {
  struct dpaw_point top_left, bottom_right;
};

struct dpaw_line {
  union {
    struct { struct dpaw_point A, B; };
    struct dpaw_point P[2];
  };
};

enum dpaw_direction {
  DPAW_DIRECTION_RIGHTWARDS,
  DPAW_DIRECTION_DOWNWARDS,
  DPAW_DIRECTION_LEFTWARDS,
  DPAW_DIRECTION_UPWARDS
};

enum dpaw_unit {
  DPAW_UNIT_PIXEL,
  DPAW_UNIT_MICROMETER
};

struct dpaw;

bool dpaw_in_rect(struct dpaw_rect rect, struct dpaw_point point);
bool dpaw_point_equal(struct dpaw_point A, struct dpaw_point B);
struct dpaw_line dpaw_line_clip(struct dpaw_rect rect, struct dpaw_line line);
struct dpaw_point dpaw_calc_distance(const struct dpaw*, struct dpaw_point A, struct dpaw_point B, enum dpaw_unit);

#endif
