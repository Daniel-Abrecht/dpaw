#ifndef DPAWINDOW_APP_H
#define DPAWINDOW_APP_H

#include <dpawindow.h>
#include <workspace.h>
#include <linked_list.h>

DECLARE_DPAW_DERIVED_WINDOW( app,
  struct dpaw_list_entry workspace_window_entry;
  struct dpaw_workspace* workspace;
  void* workspace_private;
)

int dpawindow_app_init(struct dpaw*, struct dpawindow_app*, Window);
int dpawindow_app_cleanup(struct dpawindow_app*);

#endif
