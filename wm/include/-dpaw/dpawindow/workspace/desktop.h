#ifndef WORKSPACE_HANDHELD_H
#define WORKSPACE_HANDHELD_H

#include <-dpaw/action.h>
#include <-dpaw/workspace.h>
#include <-dpaw/dpawindow/xembed.h>
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
  //struct dpaw_list desktop_window_list;
  struct dpaw_list drag_list;
  struct dpawindow_xembed xe_desktop;
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

#define DPAWINDOW_DESKTOP_WINDOW_TYPE_LIST \
  X(app_window)

enum e_dpawindow_desktop_window_type {
#define X(NAME) DPAW_DW_DESKTOP_ ## NAME,
DPAWINDOW_DESKTOP_WINDOW_TYPE_LIST
#undef X
};

struct dpawindow_desktop_window;
struct dpawindow_desktop_window_type {
  enum e_dpawindow_desktop_window_type type;
  int(*init)(struct dpawindow_desktop_window* dw);
  void(*cleanup)(struct dpawindow_desktop_window* dw);
  bool(*lookup_is_window)(struct dpawindow_desktop_window* dw, Window xwindow);
};

#define X(NAME) extern struct dpawindow_desktop_window_type dpawindow_desktop_window_type_ ## NAME;
DPAWINDOW_DESKTOP_WINDOW_TYPE_LIST
#undef X

struct dpawindow_desktop_window {
  const struct dpawindow_desktop_window_type* type;
  struct dpawindow_workspace_desktop* workspace;
  struct dpawindow_app* app_window;
};

// TODO: Split out special workspace managed windows, such as the desktop (btw. make this a normal struct with a type, and let that be a member of the frame window, which becomes the derived type)
DECLARE_DPAW_DERIVED_WINDOW( desktop_app_window,
  struct dpawindow_desktop_window dw;
  struct dpaw_list_entry desktop_entry;
  struct dpaw_list_entry drag_list_entry;
  struct dpaw_rect drag_offset;
  int drag_device;
  enum dpaw_desktop_window_drag_action drag_action;
  struct dpaw_action deferred_redraw;
  GC gc;
  struct dpaw_rect border;
  struct dpaw_rect old_boundary;
  bool has_border;
  struct dpaw_desktop_window_button button[DPAW_DESKTOP_WINDOW_BUTTON_COUNT];
)

enum event_handler_result dpaw_workspace_desktop_app_window_handle_button_press(struct dpawindow_workspace_desktop* desktop_workspace, const xev_XI_ButtonPress_t* event);

#endif
