#ifndef DPAWIN_H
#define DPAWIN_H

#include <dpawindow/root.h>

struct dpawin {
  struct dpawindow_root root;
  struct dpawindow *first, *last;
};

int dpawin_cleanup(struct dpawin*);
int dpawin_init(struct dpawin*);
int dpawin_run(struct dpawin*);
int dpawin_error_handler(Display*, XErrorEvent*);

#endif
