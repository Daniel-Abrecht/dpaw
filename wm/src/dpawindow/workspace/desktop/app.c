#include <stdio.h>
#include <-dpaw/dpaw.h>
#include <-dpaw/font.h>
#include <-dpaw/xev/X.c>
#include <-dpaw/xev/xinput2.c>
#include <-dpaw/atom/ewmh.c>
#include <-dpaw/dpawindow/workspace/desktop.h>


DEFINE_DPAW_DERIVED_WINDOW(desktop_app_window)

typedef void button_meta_onclick(struct dpawindow_desktop_app_window*);
struct button_meta {
  float width_height_ratio;
  struct dpaw_string symbol, symbol_active;
  button_meta_onclick* onclick;
};

static button_meta_onclick window_close_handler;
static button_meta_onclick window_maximize_handler;
static button_meta_onclick window_minimize_handler;

#define S(...) {__VA_ARGS__, sizeof(__VA_ARGS__)-1}
static const struct button_meta button_meta[DPAW_DESKTOP_WINDOW_BUTTON_COUNT] = {
  [DPAW_DESKTOP_WINDOW_BUTTON_CLOSE] = {
    .width_height_ratio = 1.61803,
    .symbol = S("X"), // 🗙✖✕ <- These didn't work...
    .symbol_active = S("X"), // ✖✕
    .onclick = window_close_handler,
  },
  [DPAW_DESKTOP_WINDOW_BUTTON_MAXIMIZE] = {
    .width_height_ratio = 1.30901,
    .symbol = S("□"), // 🗖  <- These didn't work...
    .symbol_active = S("V"), // 🗗 <- These didn't work...
    .onclick = window_maximize_handler,
  },
  [DPAW_DESKTOP_WINDOW_BUTTON_MINIMIZE] = {
    .width_height_ratio = 1.30901,
    .symbol = S("_"), // 🗕 <- This didn't work
    .symbol_active = S("V"), // 🗗 <- These didn't work...
    .onclick = window_minimize_handler,
  },
};
#undef S

static bool lookup_is_window(struct dpawindow_desktop_window* dw, Window xwindow);
static int init(struct dpawindow_desktop_window* dw);
static void cleanup(struct dpawindow_desktop_window* dw);
static void normalize(struct dpawindow_desktop_app_window* dw);
static void window_border_draw(struct dpawindow_desktop_app_window* dw);
static void window_border_draw_deferred(struct dpaw_action* action);
static void update_frame_content_boundary(struct dpawindow_desktop_app_window* dw, int mode);
static int window_name_change_handler(void* private, struct dpawindow_app* app, struct dpaw_string title);
static int update_frame_handler(void* private, struct dpawindow_app* app, Atom type);
static void set_focus(struct dpawindow_desktop_app_window* dw);
static bool has_to_be_framed(struct dpawindow_desktop_app_window* dw);
static void update_restore_boundary(struct dpawindow_desktop_app_window* dw);
static void ondragmove(struct dpaw_input_drag_event_owner* owner, struct dpaw_input_master_device* device, const xev_XI_Motion_t* event);

struct dpawindow_desktop_window_type dpawindow_desktop_window_type_app_window = {
  .init = init,
  .cleanup = cleanup,
  .lookup_is_window = lookup_is_window,
  .drag_handler = {
    .onmove = ondragmove,
  },
};


static int init(struct dpawindow_desktop_window* dw){
  struct dpawindow_desktop_app_window* daw = container_of(dw, struct dpawindow_desktop_app_window, dw);
  daw->window.type = &dpawindow_type_desktop_app_window;
  daw->window.dpaw = dw->workspace->window.dpaw;

  Display*const display = daw->window.dpaw->root.display;

  daw->drag_event_owner.handler = &dw->type->drag_handler;
  daw->deferred_redraw.callback = window_border_draw_deferred;
  daw->window.boundary = daw->dw.app_window->window.boundary;
  update_frame_content_boundary(daw, 0);
  struct dpaw_rect dw_boundary = daw->window.boundary;
  daw->old_boundary = dw_boundary;
  printf("%ld %ld %ld %ld\n", dw_boundary.top_left.x, dw_boundary.top_left.y, dw_boundary.bottom_right.x, dw_boundary.bottom_right.y);

  unsigned long white_pixel = WhitePixel(display, DefaultScreen(display));

  daw->window.xwindow = XCreateWindow(
    display, dw->workspace->window.xwindow,
    dw_boundary.top_left.x, dw_boundary.top_left.y, dw_boundary.bottom_right.x-dw_boundary.top_left.x, dw_boundary.bottom_right.y-dw_boundary.top_left.y, daw->has_border,
    CopyFromParent, InputOutput,
    CopyFromParent, CWBackPixel|CWBorderPixel, &(XSetWindowAttributes){
      .border_pixel = white_pixel
    }
  );
  if(!daw->window.xwindow){
    fprintf(stderr, "XCreateWindow failed\n");
    return -1;
  }

  if(dpawindow_register(&daw->window)){
    fprintf(stderr, "dpawindow_register failed\n");
    return -1;
  }

  daw->gc = XCreateGC(display, daw->window.xwindow, 0, 0);
//  XSetBackground(display, daw->gc, white_pixel);
  XSetForeground(display, daw->gc, white_pixel);
  window_border_draw(daw);

  XChangeProperty(display, daw->dw.app_window->window.xwindow, _NET_WM_ALLOWED_ACTIONS, XA_ATOM, 32, PropModeReplace, (unsigned char*)(Atom[]){
    _NET_WM_ACTION_MINIMIZE,
    _NET_WM_ACTION_MAXIMIZE_HORZ,
    _NET_WM_ACTION_MAXIMIZE_VERT,
    _NET_WM_ACTION_CLOSE,
    _NET_WM_ACTION_MOVE,
    _NET_WM_ACTION_RESIZE,
    _NET_WM_ACTION_CHANGE_DESKTOP,
  }, 7);

  for(int i=0; i<DPAW_DESKTOP_WINDOW_BUTTON_COUNT; i++){
    struct dpaw_desktop_window_button* button = &daw->button[i];
    button->xwindow = XCreateWindow(
      display, daw->window.xwindow,
      0,0,1,1, 1, // We'll set the right size & position later
      CopyFromParent, InputOutput,
      CopyFromParent, CWBackPixel|CWBorderPixel, &(XSetWindowAttributes){
        .border_pixel = white_pixel
      }
    );
    if(!button->xwindow){
      fprintf(stderr, "XCreateWindow failed for button %d\n", i);
      return -1;
    }
  }

  XReparentWindow(display, daw->dw.app_window->window.xwindow, daw->window.xwindow, 10, 30);

  printf("take_window: %lx %d %d\n", daw->dw.app_window->window.xwindow, daw->dw.app_window->observable.desired_placement.value.width, daw->dw.app_window->observable.desired_placement.value.height);
  DPAW_APP_OBSERVE(daw->dw.app_window, name, daw, window_name_change_handler);
  DPAW_APP_OBSERVE(daw->dw.app_window, type, daw, update_frame_handler);

  dpawindow_hide(&daw->window, false);
  dpawindow_set_mapping(&daw->window, true);
  set_focus(daw);

//  DPAW_CALLBACK_ADD(dpawindow, &daw->window, post_cleanup, &daw->post_cleanup);
  return 0;
}

static void cleanup(struct dpawindow_desktop_window* dw){
  struct dpawindow_desktop_app_window* daw = container_of(dw, struct dpawindow_desktop_app_window, dw);
  dpawindow_cleanup(&daw->window);
}

static void dpawindow_desktop_app_window_cleanup(struct dpawindow_desktop_app_window* dw){
  dpaw_linked_list_clear(&dw->drag_event_owner.device_owner_list);
  Display*const display = dw->window.dpaw->root.display;
  dpaw_linked_list_set(0, &dw->deferred_redraw.entry, 0);
  for(int i=0; i<DPAW_DESKTOP_WINDOW_BUTTON_COUNT; i++){
    struct dpaw_desktop_window_button* button = &dw->button[i];
    XDestroyWindow(display, button->xwindow);
  }
  XFreeGC(display, dw->gc);
  XDestroyWindow(display, dw->window.xwindow);
  dw->window.xwindow = 0;
}

static bool lookup_is_window(struct dpawindow_desktop_window* dw, Window xwindow){
  struct dpawindow_desktop_app_window* daw = container_of(dw, struct dpawindow_desktop_app_window, dw);
  return daw->window.xwindow == xwindow;
}

static void window_border_draw_deferred(struct dpaw_action* action){
  struct dpawindow_desktop_app_window* dw = container_of(action, struct dpawindow_desktop_app_window, deferred_redraw);
  Display*const display = dw->window.dpaw->root.display;
  const struct dpaw_rect appbounds = dw->dw.app_window->window.boundary;

  if(appbounds.top_left.y < 5){
    for(int i=0; i<DPAW_DESKTOP_WINDOW_BUTTON_COUNT; i++){
      struct dpaw_desktop_window_button* button = &dw->button[i];
      XUnmapWindow(display, button->xwindow);
    }
    return;
  }

  long end = appbounds.bottom_right.x;
  for(enum dpaw_desktop_window_button_e i=0; i<DPAW_DESKTOP_WINDOW_BUTTON_COUNT; i++){
    struct dpaw_desktop_window_button* button = &dw->button[i];
    const struct button_meta* meta = &button_meta[i];
    long width = appbounds.top_left.y * meta->width_height_ratio;
    XMoveResizeWindow(
      display,
      button->xwindow,
      end - width - 2,
      -1,
      width,
      appbounds.top_left.y - 1
    );
    XMapWindow(display, button->xwindow);
    end -= width + 1;
    XRectangle inc, logical;
    bool active = false;
    switch(i){
      case DPAW_DESKTOP_WINDOW_BUTTON_MAXIMIZE: active = dw->dw.app_window->wm_state._NET_WM_STATE_MAXIMIZED_VERT && dw->dw.app_window->wm_state._NET_WM_STATE_MAXIMIZED_HORZ; break;
      case DPAW_DESKTOP_WINDOW_BUTTON_MINIMIZE: active = dw->dw.app_window->wm_state._NET_WM_STATE_HIDDEN; break;
      default: break;
    }
    const struct dpaw_string symbol = active ? meta->symbol_active : meta->symbol;
    Xutf8TextExtents(dpaw_font.normal, symbol.data, symbol.size, &inc, &logical);
    long x = (width - inc.width) / 2;
    if(x < appbounds.top_left.x)
      x = appbounds.top_left.x;
    long y = appbounds.top_left.y <= inc.height ? 0 : (appbounds.top_left.y + inc.height) / 2;
    XClearWindow(display, button->xwindow);
    Xutf8DrawString(display, button->xwindow, dpaw_font.normal, dw->gc, x, y, symbol.data, symbol.size);
  }

  XClearWindow(display, dw->window.xwindow);

  struct dpaw_string name = dw->dw.app_window->observable.name.value;
  if(name.data){
    XRectangle inc, logical;
    Xutf8TextExtents(dpaw_font.normal, name.data, name.size, &inc, &logical);
    struct dpaw_rect appbounds = dw->dw.app_window->window.boundary;
    long x = (end - appbounds.top_left.x - inc.width) / 2;
    if(x < appbounds.top_left.x)
      x = appbounds.top_left.x;
    long y = appbounds.top_left.y <= inc.height ? 0 : (appbounds.top_left.y + inc.height) / 2;
    Xutf8DrawString(display, dw->window.xwindow, dpaw_font.normal, dw->gc, x, y, name.data, name.size);
  }
}

static void window_border_draw(struct dpawindow_desktop_app_window* dw){
  dpaw_linked_list_set(&dw->window.dpaw->deferred_action_list, &dw->deferred_redraw.entry, 0);
}

static bool has_to_be_framed(struct dpawindow_desktop_app_window* dw){
  Atom type = dw->dw.app_window->observable.type.value;
  if( type == _NET_WM_WINDOW_TYPE_UTILITY
   || type == _NET_WM_WINDOW_TYPE_SPLASH
   || type == _NET_WM_WINDOW_TYPE_DIALOG
   || type == _NET_WM_WINDOW_TYPE_NORMAL
  ) return true;
  return false;
}

static void update_restore_boundary(struct dpawindow_desktop_app_window* dw){
  struct dpaw_rect dw_boundary = dw->window.boundary;
  if(!dw->dw.app_window->wm_state._NET_WM_STATE_MAXIMIZED_HORZ){
    dw->old_boundary.top_left.x = dw_boundary.top_left.x;
    dw->old_boundary.bottom_right.x = dw_boundary.bottom_right.x;
  }
  if(!dw->dw.app_window->wm_state._NET_WM_STATE_MAXIMIZED_VERT){
    dw->old_boundary.top_left.y = dw_boundary.top_left.y;
    dw->old_boundary.bottom_right.y = dw_boundary.bottom_right.y;
  }
}

static void update_frame_content_boundary(struct dpawindow_desktop_app_window* daw, int mode){
  struct dpaw_rect border = {{0,0},{0,0}};
  bool has_border = has_to_be_framed(daw);
  daw->has_border = has_border;
  if(has_border){
    border.top_left.y = 30;
    if(!daw->dw.app_window->wm_state._NET_WM_STATE_MAXIMIZED_VERT){
      border.bottom_right.y = 5;
    }
    if(!daw->dw.app_window->wm_state._NET_WM_STATE_MAXIMIZED_HORZ){
      border.top_left.x = 5;
      border.bottom_right.x = 5;
    }
  }
  if(!dpaw_rect_equal(daw->border, border)){
    daw->border = border;
    XChangeProperty(daw->window.dpaw->root.display, daw->dw.app_window->window.xwindow, _NET_FRAME_EXTENTS, XA_CARDINAL, 32, PropModeReplace, (unsigned char*)(long[]){border.top_left.x,border.top_left.y,border.bottom_right.x,border.bottom_right.y}, 4);
  }
  if(mode){
    struct dpaw_rect dw_boundary = daw->window.boundary;
    update_restore_boundary(daw);
    printf("update_frame_content_boundary %c%c %ld %ld %ld %ld\n", daw->dw.app_window->wm_state._NET_WM_STATE_MAXIMIZED_HORZ?'t':'f', daw->dw.app_window->wm_state._NET_WM_STATE_MAXIMIZED_HORZ?'t':'f', daw->old_boundary.top_left.x, daw->old_boundary.top_left.y, daw->old_boundary.bottom_right.x, daw->old_boundary.bottom_right.y);
    dw_boundary.bottom_right.x = dw_boundary.bottom_right.x - dw_boundary.top_left.x - border.bottom_right.x;
    dw_boundary.bottom_right.y = dw_boundary.bottom_right.y - dw_boundary.top_left.y - border.bottom_right.y;
    dw_boundary.top_left.y = border.top_left.y;
    dw_boundary.top_left.x = border.top_left.x;
    if(mode == 1){
      dpawindow_place_window(&daw->dw.app_window->window, dw_boundary);
    }else{
      daw->dw.app_window->window.boundary = dw_boundary;
    }
  }else{
    struct dpaw_rect dw_boundary = daw->dw.app_window->window.boundary;
    dw_boundary.bottom_right.x = daw->window.boundary.top_left.x + dw_boundary.bottom_right.x - dw_boundary.top_left.x + (border.top_left.x + border.bottom_right.x);
    dw_boundary.bottom_right.y = daw->window.boundary.top_left.y + dw_boundary.bottom_right.y - dw_boundary.top_left.y + (border.top_left.y + border.bottom_right.y);
    daw->window.boundary = dw_boundary;
    update_frame_content_boundary(daw, 2);
  }
}

static void set_focus(struct dpawindow_desktop_app_window* dw){
  Display*const display = dw->window.dpaw->root.display;
  XRaiseWindow(display, dw->window.xwindow);
  XRaiseWindow(display, dw->dw.app_window->window.xwindow);
  dpaw_workspace_set_focus(dw->dw.app_window);
  dpaw_linked_list_set(&dw->dw.workspace->workspace.window_list, &dw->dw.app_window->workspace_window_entry, dw->dw.workspace->workspace.window_list.first);
}

static int window_name_change_handler(void* private, struct dpawindow_app* app, struct dpaw_string title){
  struct dpawindow_desktop_app_window* dw = private;
  (void)dw;
  (void)app;
  (void)title;
  window_border_draw(private);
  return 0;
}

static int update_frame_handler(void* private, struct dpawindow_app* app, Atom type){
  struct dpawindow_desktop_app_window* dw = private;
  (void)app;
  (void)type;
  update_frame_content_boundary(dw, 1);
  return 0;
}

EV_ON(desktop_app_window, Expose){
  if(event->y > window->dw.app_window->window.boundary.top_left.y)
    return EHR_OK;
  window_border_draw(window);
  return EHR_OK;
}

static int hide(struct dpawindow_desktop_app_window* daw){
  dpawindow_hide(&daw->window, true);
  printf("hide %lx\n", daw->dw.app_window->window.xwindow);
  daw->dw.app_window->wm_state._NET_WM_STATE_HIDDEN = true;
  dpawindow_app_update_wm_state(daw->dw.app_window);
  dpawindow_hide(&daw->dw.app_window->window, true);
  return 0;
}

void window_close_handler(struct dpawindow_desktop_app_window* daw){
  dpawindow_close(&daw->dw.app_window->window);
}

void window_minimize_handler(struct dpawindow_desktop_app_window* daw){
  hide(daw);
}

static void maximize_x(struct dpawindow_desktop_app_window* daw){
  update_restore_boundary(daw);
  daw->dw.app_window->wm_state._NET_WM_STATE_MAXIMIZED_HORZ = true;
  struct dpaw_rect workspace_boundary = daw->dw.app_window->workspace->window->boundary;
  struct dpaw_rect boundary = daw->window.boundary;
  boundary.top_left.x = -daw->has_border;
  boundary.bottom_right.x = workspace_boundary.bottom_right.x - workspace_boundary.top_left.x;
  dpawindow_place_window(&daw->window, boundary);
  update_frame_content_boundary(daw, 1);
  dpawindow_app_update_wm_state(daw->dw.app_window);
}

static void maximize_y(struct dpawindow_desktop_app_window* daw){
  update_restore_boundary(daw);
  daw->dw.app_window->wm_state._NET_WM_STATE_MAXIMIZED_VERT = true;
  struct dpaw_rect workspace_boundary = daw->dw.app_window->workspace->window->boundary;
  struct dpaw_rect boundary = daw->window.boundary;
  boundary.top_left.y = -daw->has_border;
  boundary.bottom_right.y = workspace_boundary.bottom_right.y - workspace_boundary.top_left.y;
  dpawindow_place_window(&daw->window, boundary);
  update_frame_content_boundary(daw, 1);
  dpawindow_app_update_wm_state(daw->dw.app_window);
}

static void normalize(struct dpawindow_desktop_app_window* daw){
  if(!daw->dw.app_window->wm_state._NET_WM_STATE_MAXIMIZED_VERT && !daw->dw.app_window->wm_state._NET_WM_STATE_MAXIMIZED_HORZ)
    return;
  dpawindow_place_window(&daw->window, daw->old_boundary);
  daw->dw.app_window->wm_state._NET_WM_STATE_MAXIMIZED_VERT = false;
  daw->dw.app_window->wm_state._NET_WM_STATE_MAXIMIZED_HORZ = false;
  update_frame_content_boundary(daw, 1);
  dpawindow_app_update_wm_state(daw->dw.app_window);
}

static void window_maximize_handler(struct dpawindow_desktop_app_window* daw){
  if(!(daw->dw.app_window->wm_state._NET_WM_STATE_MAXIMIZED_HORZ && daw->dw.app_window->wm_state._NET_WM_STATE_MAXIMIZED_VERT)){
    maximize_x(daw);
    maximize_y(daw);
  }else{
    normalize(daw);
  }
}

enum event_handler_result dpaw_workspace_desktop_app_window_handle_button_press(struct dpawindow_workspace_desktop* desktop_workspace, const xev_XI_ButtonPress_t* event){
  const int edge_grab_rim = 4;
//  printf("XI_ButtonPress %lf %lf %lf %lf\n", event->root_x, event->root_y, event->event_x, event->event_y);
  struct dpaw_point point = {event->event_x, event->event_y};
  struct dpawindow_desktop_app_window* match = 0;
  // TODO: Make a list of just the dpawindow_desktop_app_window
  for(struct dpaw_list_entry* it=desktop_workspace->workspace.window_list.first; it; it=it->next){
    struct dpawindow_app* app = container_of(it, struct dpawindow_app, workspace_window_entry);
    struct dpawindow_desktop_window* dw = app->workspace_private;
    if(!dw || dw->type != &dpawindow_desktop_window_type_app_window)
      continue;
    struct dpawindow_desktop_app_window* daw = container_of(dw, struct dpawindow_desktop_app_window, dw);
    if(!has_to_be_framed(daw))
      continue;
    bool in_super_area = !match && dpaw_in_rect((struct dpaw_rect){
      .top_left.x     = daw->window.boundary.top_left.x     - edge_grab_rim,
      .top_left.y     = daw->window.boundary.top_left.y     - edge_grab_rim,
      .bottom_right.x = daw->window.boundary.bottom_right.x + edge_grab_rim,
      .bottom_right.y = daw->window.boundary.bottom_right.y + edge_grab_rim,
    }, point);
    bool in_frame = in_super_area
                 && !dpaw_in_rect(daw->dw.app_window->window.boundary, (struct dpaw_point){point.x-daw->window.boundary.top_left.x,point.y-daw->window.boundary.top_left.y})
                 &&  dpaw_in_rect(daw->window.boundary, point);
    if(in_super_area || in_frame)
      match = daw;
    if(in_frame)
      break;
  }
  bool grab = false;
  if(match){
    set_focus(match);
    match->drag_offset.top_left = (struct dpaw_point){point.x - match->window.boundary.top_left.x, point.y - match->window.boundary.top_left.y};
    match->drag_offset.bottom_right.x = match->window.boundary.bottom_right.x - match->window.boundary.top_left.x - match->drag_offset.top_left.x;
    match->drag_offset.bottom_right.y = match->window.boundary.bottom_right.y - match->window.boundary.top_left.y - match->drag_offset.top_left.y;
    match->drag_action = 0;
    struct dpaw_rect dw_bounds = match->dw.app_window->window.boundary;
    if(dw_bounds.top_left.y > 5 && match->drag_offset.top_left.y <= dw_bounds.top_left.y){
      long end = dw_bounds.bottom_right.x;
      for(enum dpaw_desktop_window_button_e i=0; i<DPAW_DESKTOP_WINDOW_BUTTON_COUNT; i++){
        const struct button_meta* meta = &button_meta[i];
        long width = dw_bounds.top_left.y * meta->width_height_ratio;
        long start = end - width - 1;
        if(match->drag_offset.top_left.x >= start && match->drag_offset.top_left.x < end){
          button_meta[i].onclick(match);
          return EHR_OK;
        }
        end = start;
      }
    }
    if(match->drag_offset.top_left.y <= DPAW_MIN(dw_bounds.top_left.y,edge_grab_rim))
      match->drag_action |= DPAW_DW_DRAG_TOP;
    if(match->drag_offset.top_left.x <= DPAW_MAX(dw_bounds.top_left.x,edge_grab_rim))
      match->drag_action |= DPAW_DW_DRAG_LEFT;
    if(match->drag_offset.bottom_right.y <= DPAW_MAX(match->window.boundary.bottom_right.y-match->window.boundary.top_left.y-dw_bounds.bottom_right.y,edge_grab_rim))
      match->drag_action |= DPAW_DW_DRAG_BOTTOM;
    if(match->drag_offset.bottom_right.x <= DPAW_MAX(match->window.boundary.bottom_right.x-match->window.boundary.top_left.x-dw_bounds.bottom_right.x,edge_grab_rim))
      match->drag_action |= DPAW_DW_DRAG_RIGHT;
    if(match->drag_action || match->drag_offset.top_left.y <= dw_bounds.top_left.y)
      grab = true;
//    printf("XI_ButtonPress %d %lf %lf %lf %lf\n", match->drag_action, event->root_x, event->root_y, event->event_x, event->event_y);
  }
  if(grab){
    printf("dpaw_own_drag_event\n");
    dpaw_own_drag_event(desktop_workspace->workspace.window->dpaw, event, &match->drag_event_owner);
    return EHR_OK;
  }
  if(!match)
    return EHR_NEXT;
  return EHR_UNHANDLED;
}

static void ondragmove(struct dpaw_input_drag_event_owner* owner, struct dpaw_input_master_device* device, const xev_XI_Motion_t* event){
  (void)device;
//  printf("ondragmove %lf %lf %lf %lf\n", event->root_x, event->root_y, event->event_x, event->event_y);
  struct dpaw_point point = {event->event_x, event->event_y};
  struct dpawindow_desktop_app_window* dw = container_of(owner, struct dpawindow_desktop_app_window, drag_event_owner);
  switch(dw->drag_action){
    case DPAW_DW_DRAG_MIDDLE      : dpawindow_move_to(&dw->window, (struct dpaw_point){
      point.x-dw->drag_offset.top_left.x, point.y-dw->drag_offset.top_left.y
    }); break;
    case DPAW_DW_DRAG_TOP         : dpawindow_place_window(&dw->window, (struct dpaw_rect){
      .top_left.y = point.y - dw->drag_offset.top_left.y,
      .top_left.x = dw->window.boundary.top_left.x,
      .bottom_right = dw->window.boundary.bottom_right
    }); break;
    case DPAW_DW_DRAG_LEFT        : dpawindow_place_window(&dw->window, (struct dpaw_rect){
      .top_left.x = point.x - dw->drag_offset.top_left.x,
      .top_left.y = dw->window.boundary.top_left.y,
      .bottom_right = dw->window.boundary.bottom_right
    }); break;
    case DPAW_DW_DRAG_RIGHT       : dpawindow_place_window(&dw->window, (struct dpaw_rect){
      .top_left = dw->window.boundary.top_left,
      .bottom_right.y = dw->window.boundary.bottom_right.y,
      .bottom_right.x = point.x + dw->drag_offset.bottom_right.x,
    }); break;
    case DPAW_DW_DRAG_BOTTOM      : dpawindow_place_window(&dw->window, (struct dpaw_rect){
      .top_left = dw->window.boundary.top_left,
      .bottom_right.x = dw->window.boundary.bottom_right.x,
      .bottom_right.y = point.y + dw->drag_offset.bottom_right.y,
    }); break;
    case DPAW_DW_DRAG_TOP_LEFT    : dpawindow_place_window(&dw->window, (struct dpaw_rect){
      .top_left.x = point.x - dw->drag_offset.top_left.x,
      .top_left.y = point.y - dw->drag_offset.top_left.y,
      .bottom_right = dw->window.boundary.bottom_right
    }); break;
    case DPAW_DW_DRAG_TOP_RIGHT   : dpawindow_place_window(&dw->window, (struct dpaw_rect){
      .top_left.y = point.y - dw->drag_offset.top_left.y,
      .top_left.x = dw->window.boundary.top_left.x,
      .bottom_right.y = dw->window.boundary.bottom_right.y,
      .bottom_right.x = point.x + dw->drag_offset.bottom_right.x,
    }); break;
    case DPAW_DW_DRAG_BOTTOM_LEFT :dpawindow_place_window(&dw->window, (struct dpaw_rect){
      .top_left.y = dw->window.boundary.top_left.y,
      .top_left.x = point.x - dw->drag_offset.top_left.x,
      .bottom_right.x = dw->window.boundary.bottom_right.x,
      .bottom_right.y = point.y + dw->drag_offset.bottom_right.y,
    }); break;
    case DPAW_DW_DRAG_BOTTOM_RIGHT: dpawindow_place_window(&dw->window, (struct dpaw_rect){
      .top_left = dw->window.boundary.top_left,
      .bottom_right.x = point.x + dw->drag_offset.bottom_right.x,
      .bottom_right.y = point.y + dw->drag_offset.bottom_right.y,
    }); break;
  }
  bool wm_state_changed = false;
  if(dw->drag_action & (DPAW_DW_DRAG_LEFT|DPAW_DW_DRAG_RIGHT)){
    wm_state_changed |= dw->dw.app_window->wm_state._NET_WM_STATE_MAXIMIZED_HORZ;
    dw->dw.app_window->wm_state._NET_WM_STATE_MAXIMIZED_HORZ = false;
  }
  if(dw->drag_action & (DPAW_DW_DRAG_TOP|DPAW_DW_DRAG_BOTTOM)){
    wm_state_changed |= dw->dw.app_window->wm_state._NET_WM_STATE_MAXIMIZED_VERT;
    dw->dw.app_window->wm_state._NET_WM_STATE_MAXIMIZED_VERT = false;
  }
  if(dw->drag_action == DPAW_DW_DRAG_MIDDLE)
    normalize(dw);
  if(wm_state_changed)
    dpawindow_app_update_wm_state(dw->dw.app_window);
  if(dw->drag_action != DPAW_DW_DRAG_MIDDLE || wm_state_changed){
    update_frame_content_boundary(dw, 1);
  }else{
    update_restore_boundary(dw);
  }
}
