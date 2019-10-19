#ifndef PRIMITIVES_H
#define PRIMITIVES_H

struct dpawin_point {
  unsigned x, y;
};

struct dpawin_rect {
  struct dpawin_point position;
  struct dpawin_point size;
};

#endif
