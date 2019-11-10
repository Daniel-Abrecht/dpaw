#ifndef WORKSPACE_HANDHELD_H
#define WORKSPACE_HANDHELD_H

#include <workspace.h>
#include <stdbool.h>
#include <touch_gesture_manager.h>
#include <touch_gesture_detector/sideswipe.h>

DECLARE_DPAWIN_WORKSPACE( handheld,
  struct dpawindow_handheld_window* current;
  struct dpawin_sideswipe_detector sideswipe;
  struct dpawin_sideswipe_detector_params sideswipe_params;
  struct dpawin_touch_gesture_manager touch_gesture_manager;
)

struct dpawindow_handheld_window {
  struct dpawindow_app* app_window;
  struct dpawindow_workspace_handheld* workspace;
};

#endif
