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

DECLARE_DPAW_DERIVED_WINDOW( desktop_window,
  struct dpaw_list_entry desktop_entry;
  struct dpawindow_app* app_window; // This is just the applications window / the content
  struct dpawindow_workspace_desktop* workspace;
  bool has_border;
)

#endif
