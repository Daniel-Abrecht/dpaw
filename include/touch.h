#ifndef TOUCH_H
#define TOUCH_H

#include <X11/Xlib.h>

struct dpawin_touch_manager {
  struct dpawin* dpawin;
};

int dpawin_touch_init(struct dpawin_touch_manager*, struct dpawin*);

#endif
