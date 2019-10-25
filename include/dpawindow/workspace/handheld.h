#ifndef WORKSPACE_HANDHELD_H
#define WORKSPACE_HANDHELD_H

#include <workspace.h>

DECLARE_DPAWIN_WORKSPACE( handheld,
  struct dpawindow_handheld_window* current;
)

struct dpawindow_handheld_window {
  struct dpawindow_app* app_window;
  struct dpawindow_workspace_handheld* workspace;
};

#endif
