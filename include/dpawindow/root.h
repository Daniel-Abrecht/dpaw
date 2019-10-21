#ifndef DPAROOTWINDOW_H
#define DPAROOTWINDOW_H

#include <dpawindow.h>
#include <workspace.h>

DECLARE_DPAWIN_DERIVED_WINDOW( root,
  Display* display;
  struct dpawin_workspace_manager workspace_manager;
)

int dpawindow_root_init(struct dpawindow_root*);
int dpawindow_root_cleanup(struct dpawindow_root* window);

#endif
