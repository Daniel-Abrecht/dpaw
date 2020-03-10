#ifndef DPAWINDOW_ROOT_H
#define DPAWINDOW_ROOT_H

#include <dpaw/dpawindow.h>
#include <dpaw/workspace.h>
#include <dpaw/screenchange.h>

DECLARE_DPAW_DERIVED_WINDOW( root,
  Display* display;
  struct dpaw_workspace_manager workspace_manager;
  struct dpaw_screenchange_detector screenchange_detector;
)

int dpawindow_root_init(struct dpaw*, struct dpawindow_root*);

#endif
