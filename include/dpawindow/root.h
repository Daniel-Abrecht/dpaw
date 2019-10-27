#ifndef DPAWINDOW_ROOT_H
#define DPAWINDOW_ROOT_H

#include <touch.h>
#include <dpawindow.h>
#include <workspace.h>
#include <screenchange.h>

DECLARE_DPAWIN_DERIVED_WINDOW( root,
  Display* display;
  struct dpawin_xev* xev_list;
  struct dpawin_workspace_manager workspace_manager;
  struct dpawin_screenchange_detector screenchange_detector;
  struct dpawin_touch_manager touch_manager;
)

int dpawindow_root_init(struct dpawin*, struct dpawindow_root*);
int dpawindow_root_cleanup(struct dpawindow_root*);

#endif
