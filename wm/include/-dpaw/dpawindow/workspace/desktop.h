#ifndef WORKSPACE_HANDHELD_H
#define WORKSPACE_HANDHELD_H

#include <-dpaw/workspace.h>
#include <-dpaw/dpawindow/xembed.h>
#include <-dpaw/touch_gesture_manager.h>
#include <-dpaw/touch_gesture_detector/line.h>
#include <-dpaw/touch_gesture_detector/sideswipe.h>
#include <stdbool.h>

DECLARE_DPAW_WORKSPACE( desktop,
  struct dpaw_list desktop_window_list;
)

struct dpawindow_desktop_window {
  struct dpaw_list_entry desktop_entry;
  struct dpawindow_app* app_window;
  struct dpawindow_workspace_desktop* workspace;
};

#endif
