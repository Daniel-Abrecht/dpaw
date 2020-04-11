#ifndef DPAWINDOW_ROOT_H
#define DPAWINDOW_ROOT_H

#include <dpaw/dpawindow.h>
#include <dpaw/workspace.h>
#include <dpaw/screenchange.h>

typedef struct dpawindow_root dpawindow_root;
DPAW_DECLARE_CALLBACK_TYPE(dpawindow_root)

DECLARE_DPAW_DERIVED_WINDOW( root,
  Display* display;
  struct dpaw_workspace_manager workspace_manager;
  struct dpaw_screenchange_detector screenchange_detector;
  struct dpaw_callback_list_dpawindow_root window_mapped;
)

int dpawindow_root_init(struct dpaw*, struct dpawindow_root*);

#endif
