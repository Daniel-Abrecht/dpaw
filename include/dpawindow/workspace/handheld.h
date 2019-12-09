#ifndef WORKSPACE_HANDHELD_H
#define WORKSPACE_HANDHELD_H

#include <workspace.h>
#include <stdbool.h>
#include <touch_gesture_manager.h>
#include <touch_gesture_detector/sideswipe.h>

enum dpawindow_handheld_window_type {
  DPAWINDOW_HANDHELD_UNSET,
  DPAWINDOW_HANDHELD_NORMAL,
  DPAWINDOW_HANDHELD_TOP_DOCK,
  DPAWINDOW_HANDHELD_BOTTOM_DOCK
};

DECLARE_DPAW_WORKSPACE( handheld,
  struct dpaw_sideswipe_detector sideswipe;
  struct dpaw_sideswipe_detector_params sideswipe_params;
  struct dpaw_touch_gesture_manager touch_gesture_manager;
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
