#ifndef WORKSPACE_HANDHELD_H
#define WORKSPACE_HANDHELD_H

#include <-dpaw/workspace.h>
#include <-dpaw/dpawindow/xembed.h>
#include <-dpaw/touch_gesture_manager.h>
#include <-dpaw/touch_gesture_detector/line.h>
#include <-dpaw/touch_gesture_detector/sideswipe.h>
#include <stdbool.h>

enum dpawindow_handheld_window_type {
  DPAWINDOW_HANDHELD_UNSET,
  DPAWINDOW_HANDHELD_NORMAL,
  DPAWINDOW_HANDHELD_TOP_DOCK,
  DPAWINDOW_HANDHELD_BOTTOM_DOCK
};

DECLARE_DPAW_WORKSPACE( handheld,
  struct dpaw_sideswipe_detector sideswipe;
  struct dpaw_line_touch_detector keyboard_top_boundary;
  struct dpaw_touch_gesture_manager touch_gesture_manager;
  struct dpawindow_xembed keyboard;
  struct dpaw_list handheld_window_list;
  struct dpawindow_handheld_window *current;
  struct dpawindow_handheld_window *top_dock, *bottom_dock;
)

struct dpawindow_handheld_window {
  struct dpaw_list_entry handheld_entry;
  enum dpawindow_handheld_window_type type;
  struct dpawindow_app* app_window;
  struct dpawindow_workspace_handheld* workspace;
  bool active;
};

#endif
