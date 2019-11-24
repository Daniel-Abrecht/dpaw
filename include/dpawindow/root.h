#ifndef DPAWINDOW_ROOT_H
#define DPAWINDOW_ROOT_H

#include <dpawindow.h>
#include <workspace.h>
#include <screenchange.h>

DECLARE_DPAW_DERIVED_WINDOW( root,
  Display* display;
  struct dpaw_workspace_manager workspace_manager;
  struct dpaw_screenchange_detector screenchange_detector;
)

int dpawindow_root_init(struct dpaw*, struct dpawindow_root*);
int dpawindow_root_cleanup(struct dpawindow_root*);

#endif
