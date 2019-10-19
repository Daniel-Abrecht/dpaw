#ifndef DPAROOTWINDOW_H
#define DPAROOTWINDOW_H

#include <dpawindow.h>

DECLARE_DPAWIN_DERIVED_WINDOW( root,
  Display* display;
)

int dpawindow_root_init(struct dpawindow_root*);
int dpawindow_root_cleanup(struct dpawindow_root* window);

#endif
