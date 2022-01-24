#ifndef WORKSPACE_HANDHELD_H
#define WORKSPACE_HANDHELD_H

#include <-dpaw/action.h>
#include <-dpaw/workspace.h>
#include <-dpaw/dpawindow/xembed.h>
#include <-dpaw/touch_gesture_manager.h>
#include <-dpaw/touch_gesture_detector/line.h>
#include <-dpaw/touch_gesture_detector/sideswipe.h>
#include <stdbool.h>

enum dpaw_desktop_window_drag_action {
  DPAW_DW_DRAG_MIDDLE = 0,
  DPAW_DW_DRAG_TOP    = 1,
  DPAW_DW_DRAG_LEFT   = 2,
  DPAW_DW_DRAG_RIGHT  = 4,
  DPAW_DW_DRAG_BOTTOM = 8,
  DPAW_DW_DRAG_TOP_LEFT     = DPAW_DW_DRAG_TOP    | DPAW_DW_DRAG_LEFT ,
  DPAW_DW_DRAG_TOP_RIGHT    = DPAW_DW_DRAG_TOP    | DPAW_DW_DRAG_RIGHT,
  DPAW_DW_DRAG_BOTTOM_LEFT  = DPAW_DW_DRAG_BOTTOM | DPAW_DW_DRAG_LEFT ,
  DPAW_DW_DRAG_BOTTOM_RIGHT = DPAW_DW_DRAG_BOTTOM | DPAW_DW_DRAG_RIGHT,
};

DECLARE_DPAW_WORKSPACE( desktop,
  struct dpaw_list desktop_window_list;
  struct dpaw_list drag_list;
)

struct dpaw_desktop_window_button {
  Window xwindow;
};

enum dpaw_desktop_window_button_e {
  DPAW_DESKTOP_WINDOW_BUTTON_CLOSE,
  DPAW_DESKTOP_WINDOW_BUTTON_MAXIMIZE,
  DPAW_DESKTOP_WINDOW_BUTTON_MINIMIZE,
  DPAW_DESKTOP_WINDOW_BUTTON_COUNT
};

DECLARE_DPAW_DERIVED_WINDOW( desktop_window,
  struct dpaw_list_entry desktop_entry;
  struct dpawindow_app* app_window; // This is just the applications window / the content
  struct dpawindow_workspace_desktop* workspace;
  struct dpaw_list_entry drag_list_entry;
  struct dpaw_rect drag_offset;
  int drag_device;
  enum dpaw_desktop_window_drag_action drag_action;
  struct dpaw_action deferred_redraw;
  GC gc;
  struct dpaw_rect border;
  struct dpaw_rect old_boundary;
  bool has_border, configured;
  struct dpaw_desktop_window_button button[DPAW_DESKTOP_WINDOW_BUTTON_COUNT];
)

#endif
