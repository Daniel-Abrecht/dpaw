#ifndef PRIMITIVES_H
#define PRIMITIVES_H

struct dpawin_point {
  long x, y;
};

struct dpawin_rect {
  struct dpawin_point top_left;
  struct dpawin_point bottom_right;
};

enum dpawin_direction {
  DPAWIN_DIRECTION_RIGHTWARDS,
  DPAWIN_DIRECTION_DOWNWARDS,
  DPAWIN_DIRECTION_LEFTWARDS,
  DPAWIN_DIRECTION_UPWARDS
};

#endif
