#include <-dpaw/dpaw.h>
#include <-dpaw/xev/X.c>
#include <-dpaw/xev/xinput2.c>
#include <-dpaw/atom/ewmh.c>
#include <-dpaw/atom/icccm.c>
#include <-dpaw/font.h>
#include <-dpaw/dpawindow.h>
#include <-dpaw/dpawindow/app.h>
#include <-dpaw/dpawindow/root.h>
#include <-dpaw/dpawindow/workspace/desktop.h>
#include <-dpaw/touch_gesture_detector/sideswipe.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

DEFINE_DPAW_DERIVED_WINDOW(desktop_window)

static struct dpawindow_desktop_window* lookup_xwindow(struct dpawindow_workspace_desktop* desktop_workspace, Window xwindow){
  for(struct dpaw_list_entry* it=desktop_workspace->workspace.window_list.first; it; it=it->next){
    struct dpawindow_app* app = container_of(it, struct dpawindow_app, workspace_window_entry);
    struct dpawindow_desktop_window* dw = app->workspace_private;
    if(!dw) continue;
    if(app->window.xwindow == xwindow)
      return dw;
    if(dw->window.xwindow == xwindow)
      return dw;
  }
  return 0;
}

static int init(struct dpawindow_workspace_desktop* workspace){
  puts("desktop_workspace init");

  workspace->window.type = &dpawindow_type_workspace_desktop;
  workspace->window.dpaw = workspace->workspace.workspace_manager->dpaw;
  if(dpawindow_register(&workspace->window)){
    fprintf(stderr, "dpawindow_register failed\n");
    return -1;
  }

  return 0;
}

static void dpawindow_workspace_desktop_cleanup(struct dpawindow_workspace_desktop* workspace){
  (void)workspace;
  puts("desktop_workspace cleanup");
}

static int screen_added(struct dpawindow_workspace_desktop* workspace, struct dpaw_workspace_screen* screen){
  (void)workspace;
  (void)screen;
  puts("desktop_workspace screen_added");
  return 0;
}

static int screen_changed(struct dpawindow_workspace_desktop* workspace, struct dpaw_workspace_screen* screen){
  (void)workspace;
  (void)screen;
  puts("desktop_workspace screen_changed");
  return 0;
}

static void screen_removed(struct dpawindow_workspace_desktop* workspace, struct dpaw_workspace_screen* screen){
  (void)workspace;
  (void)screen;
  puts("desktop_workspace screen_removed");
}

static int screen_make_bid(struct dpawindow_workspace_desktop* workspace, struct dpaw_workspace_screen* screen){
  int bid = 0;
  const struct dpaw_screen_info* sinfo = screen->info;
  unsigned long screen_width  = sinfo->boundary.bottom_right.x - sinfo->boundary.top_left.x;
  unsigned long screen_height = sinfo->boundary.bottom_right.y - sinfo->boundary.top_left.y;
  if(screen_width >= screen_height)
    bid = 1;
  if(workspace && bid)
    bid += 1;
  printf("desktop_workspace screen_make_bid -> %d\n", bid);
  return bid;
}

static int unshow_window(struct dpawindow_desktop_window* dwin){
  if(!dwin)
    return 0;
  printf("unshow_window %lx\n", dwin->app_window->window.xwindow);
  dpawindow_hide(&dwin->app_window->window, true);
  dpawindow_hide(&dwin->window, true);
  return 0;
}

static void abandon_window(struct dpawindow_app* window){
  printf("abandon_window %lx\n", window->window.xwindow);
  struct dpawindow_desktop_window* dw = window->workspace_private;
  dpawindow_hide(&window->window, true);
  if(dw) unshow_window(dw);
  XDeleteProperty(window->window.dpaw->root.display, window->window.xwindow, _NET_FRAME_EXTENTS);
  XDeleteProperty(window->window.dpaw->root.display, window->window.xwindow, _NET_WM_ALLOWED_ACTIONS);
  if(dw) dpawindow_cleanup(&dw->window);
  window->workspace_private = 0;
}

static int desired_placement_change_handler(void* private, struct dpawindow_app* app, XSizeHints size){
  (void)private;
  (void)size;
  struct dpawindow_desktop_window* desktop_window = app->workspace_private;
  (void)desktop_window;
  printf("desired_placement_change_handler %ld %dx%d\n", app->window.xwindow, app->observable.desired_placement.value.width, app->observable.desired_placement.value.height);
  //update_window_area(desktop_window);
  //update_window_size(desktop_window->workspace);
  return 0;
}

static void dpawindow_desktop_window_cleanup(struct dpawindow_desktop_window* dw){
  dpaw_linked_list_set(0, &dw->drag_list_entry, 0);
  XDestroyWindow(dw->window.dpaw->root.display, dw->window.xwindow);
  dw->window.xwindow = 0;
  XFreeGC(dw->window.dpaw->root.display, dw->gc);
}

static void update_frame_content_boundary(struct dpawindow_desktop_window* dw, int mode){
  struct dpaw_rect border = {{0,0},{0,0}};
  if(dw->has_border){
    border = (struct dpaw_rect){{5,30},{5,5}};
  }
  if(mode){
    struct dpaw_rect dw_boundary = dw->window.boundary;
    dw_boundary.bottom_right.x = dw_boundary.bottom_right.x - dw_boundary.top_left.x - border.bottom_right.x;
    dw_boundary.bottom_right.y = dw_boundary.bottom_right.y - dw_boundary.top_left.y - border.bottom_right.y;
    dw_boundary.top_left.y = border.top_left.y;
    dw_boundary.top_left.x = border.top_left.x;
    if(mode == 1){
      dpawindow_place_window(&dw->app_window->window, dw_boundary);
    }else{
      dw->app_window->window.boundary = dw_boundary;
    }
  }else{
    struct dpaw_rect dw_boundary = dw->app_window->window.boundary;
    dw_boundary.bottom_right.x = dw->window.boundary.top_left.x + dw_boundary.bottom_right.x - dw_boundary.top_left.x + (border.top_left.x + border.bottom_right.x);
    dw_boundary.bottom_right.y = dw->window.boundary.top_left.y + dw_boundary.bottom_right.y - dw_boundary.top_left.y + (border.top_left.y + border.bottom_right.y);
    dw->window.boundary = dw_boundary;
    update_frame_content_boundary(dw, 2);
  }
}

static int window_border_draw(struct dpawindow_desktop_window* dw){
  Display*const display = dw->window.dpaw->root.display;

  XClearWindow(display, dw->window.xwindow);

  struct dpaw_string name = dw->app_window->observable.name.value;
  if(name.data){
    XRectangle inc, logical;
    Xutf8TextExtents(dpaw_font.normal, name.data, name.size, &inc, &logical);
    struct dpaw_rect appbounds = dw->app_window->window.boundary;
    int x = (appbounds.bottom_right.x - appbounds.top_left.x - inc.width) / 2;
    int y = (appbounds.top_left.y) / 2 + inc.height / 2;
    Xutf8DrawString(display, dw->window.xwindow, dpaw_font.normal, dw->gc, x, y, name.data, name.size);
  }

  return 0;
}

static int window_name_change_handler(void* private, struct dpawindow_app* app, struct dpaw_string title){
  (void)private;
  (void)title;
  window_border_draw(app->workspace_private);
  return 0;
}

EV_ON(desktop_window, Expose){
  window_border_draw(window);
  return EHR_OK;
}

static int init_desktop_window(struct dpawindow_desktop_window* dw, struct dpawindow_app* app){
  dw->window.type = &dpawindow_type_desktop_window;
  dw->window.dpaw = dw->workspace->window.dpaw;
  Display*const display = dw->window.dpaw->root.display;
  dw->app_window = app;
  app->workspace_private = dw;

  dw->has_border = true;
  dw->window.boundary = dw->app_window->window.boundary;
  update_frame_content_boundary(dw, 0);
  struct dpaw_rect dw_boundary = dw->window.boundary;
  printf("%ld %ld %ld %ld\n", dw_boundary.top_left.x, dw_boundary.top_left.y, dw_boundary.bottom_right.x, dw_boundary.bottom_right.y);

  unsigned long white_pixel = WhitePixel(display, DefaultScreen(display));

  dw->window.xwindow = XCreateWindow(
    display, dw->workspace->window.xwindow,
    dw_boundary.top_left.x, dw_boundary.top_left.y, dw_boundary.bottom_right.x-dw_boundary.top_left.x, dw_boundary.bottom_right.y-dw_boundary.top_left.y, dw->has_border,
    CopyFromParent, InputOutput,
    CopyFromParent, CWBackPixel|CWBorderPixel, &(XSetWindowAttributes){
      .border_pixel = white_pixel
    }
  );
  if(!dw->window.xwindow){
    fprintf(stderr, "XCreateWindow failed\n");
    return -1;
  }

  if(dpawindow_register(&dw->window)){
    fprintf(stderr, "dpawindow_register failed\n");
    return -1;
  }

  dw->gc = XCreateGC(display, dw->window.xwindow, 0, 0);
//  XSetBackground(display, dw->gc, white_pixel);
  XSetForeground(display, dw->gc, white_pixel);
  window_border_draw(dw);

  XChangeProperty(display, dw->app_window->window.xwindow, _NET_FRAME_EXTENTS, XA_CARDINAL, 32, PropModeReplace, (unsigned char*)(long[]){0,0,0,0}, 4);
  XChangeProperty(display, dw->app_window->window.xwindow, _NET_WM_ALLOWED_ACTIONS, XA_ATOM, 32, PropModeReplace, (unsigned char*)(Atom[]){_NET_WM_ACTION_CLOSE}, 1);

  XReparentWindow(display, app->window.xwindow, dw->window.xwindow, 10, 30);

  printf("take_window: %lx %d %d\n", dw->app_window->window.xwindow, dw->app_window->observable.desired_placement.value.width, dw->app_window->observable.desired_placement.value.height);
  DPAW_APP_OBSERVE(dw->app_window, desired_placement, 0, desired_placement_change_handler);
  DPAW_APP_OBSERVE(dw->app_window, name, 0, window_name_change_handler);

  return 0;
}

static void set_focus(struct dpawindow_desktop_window* dw){
  Display*const display = dw->window.dpaw->root.display;
  XRaiseWindow(display, dw->window.xwindow);
  XRaiseWindow(display, dw->app_window->window.xwindow);
  dpaw_workspace_set_focus(dw->app_window);
  dpaw_linked_list_set(&dw->workspace->workspace.window_list, &dw->app_window->workspace_window_entry, dw->workspace->workspace.window_list.first);
}

static int take_window(struct dpawindow_workspace_desktop* workspace, struct dpawindow_app* app){
  printf("take_window %lx\n", app->window.xwindow);

  struct dpawindow_desktop_window* dw = calloc(sizeof(struct dpawindow_desktop_window), 1);
  if(!dw)
    return -1;
  dw->workspace = workspace;

  if(init_desktop_window(dw, app)){
    printf("init_desktop_window failed");
    dpawindow_cleanup(&dw->window);
    return -1;
  }

  dpawindow_hide(&dw->window, false);
  dpawindow_set_mapping(&dw->window, true);
  dpawindow_hide(&app->window, false);
  dpawindow_set_mapping(&app->window, true);
  set_focus(dw);

  return 0;
}

EV_ON(workspace_desktop, ClientMessage){
  struct dpawindow_desktop_window* child = lookup_xwindow(window, event->window);
  if(!child)
    return EHR_UNHANDLED;
  if(event->message_type == WM_PROTOCOLS)
    if((Atom)event->data.l[0] == WM_DELETE_WINDOW)
      unshow_window(child);
  return EHR_NEXT;
}

static enum event_handler_result configure_request_handler(struct dpawindow_desktop_window* child, xev_ConfigureRequest_t* event){
  if(event->value_mask & CWWidth)
    child->app_window->observable.desired_placement.value.width = event->width;
  if(event->value_mask & CWHeight)
    child->app_window->observable.desired_placement.value.height = event->height;
  if(event->value_mask & CWX)
    child->app_window->observable.desired_placement.value.x = event->x;
  if(event->value_mask & CWY)
    child->app_window->observable.desired_placement.value.y = event->y;
  update_frame_content_boundary(child, 2);
  struct dpaw_rect fbounds = child->window.boundary;
  struct dpaw_rect wbounds = child->app_window->window.boundary;
//  printf("ConfigureRequest %lx %s\n", event->window, event->window == child->window.xwindow ? "frame" : "app");
//  printf("%d %d %d %d\n", event->x, event->y, event->width, event->height);
//  printf("%ld %ld %ld %ld\n", fbounds.top_left.x, fbounds.top_left.y, fbounds.bottom_right.x, fbounds.bottom_right.y);
//  printf("%ld %ld %ld %ld\n", wbounds.top_left.x, wbounds.top_left.y, wbounds.bottom_right.x, wbounds.bottom_right.y);
//  printf("%d %d %d %d\n", child->app_window->observable.desired_placement.value.x, child->app_window->observable.desired_placement.value.y, child->app_window->observable.desired_placement.value.width, child->app_window->observable.desired_placement.value.height);
  if(event->value_mask & (CWWidth|CWHeight|CWX|CWY))
    event->value_mask |= (CWWidth|CWHeight|CWX|CWY);
  XConfigureWindow(child->window.dpaw->root.display, child->app_window->window.xwindow, event->value_mask, &(XWindowChanges){
    .x = wbounds.top_left.x,
    .y = wbounds.top_left.y,
    .width  = DPAW_MAX(wbounds.bottom_right.x - wbounds.top_left.x, 1),
    .height = DPAW_MAX(wbounds.bottom_right.y - wbounds.top_left.y, 1),
    .border_width = event->border_width,
    .sibling      = event->above,
    .stack_mode   = event->detail
  });
  if(event->value_mask & (CWWidth|CWHeight|CWX|CWY)){
    XConfigureWindow(child->window.dpaw->root.display, child->window.xwindow, event->value_mask, &(XWindowChanges){
      .x = fbounds.top_left.x,
      .y = fbounds.top_left.y,
      .width  = DPAW_MAX(fbounds.bottom_right.x - fbounds.top_left.x, 1),
      .height = DPAW_MAX(fbounds.bottom_right.y - fbounds.top_left.y, 1),
    });
    DPAW_APP_OBSERVABLE_NOTIFY(child->app_window, desired_placement);
  }
  return EHR_OK;
}

EV_ON(desktop_window, ConfigureRequest){
  return configure_request_handler(window, event);
}

EV_ON(workspace_desktop, ConfigureRequest){
  struct dpawindow_desktop_window* child = lookup_xwindow(window, event->window);
  if(!child)
    return EHR_UNHANDLED;
  return configure_request_handler(child, event);
}

EV_ON(workspace_desktop, XI_ButtonPress){
  const int edge_grab_rim = 4;
//  printf("XI_ButtonPress %lf %lf %lf %lf\n", event->root_x, event->root_y, event->event_x, event->event_y);
  struct dpaw_point point = {event->event_x, event->event_y};
  struct dpawindow_desktop_window* match = 0;
  for(struct dpaw_list_entry* it=window->workspace.window_list.first; it; it=it->next){
    struct dpawindow_app* app = container_of(it, struct dpawindow_app, workspace_window_entry);
    struct dpawindow_desktop_window* dw = app->workspace_private;
    bool in_super_area = !match && dpaw_in_rect((struct dpaw_rect){
      .top_left.x     = dw->window.boundary.top_left.x     - edge_grab_rim,
      .top_left.y     = dw->window.boundary.top_left.y     - edge_grab_rim,
      .bottom_right.x = dw->window.boundary.bottom_right.x + edge_grab_rim,
      .bottom_right.y = dw->window.boundary.bottom_right.y + edge_grab_rim,
    }, point);
    bool in_frame = in_super_area
                 && !dpaw_in_rect(dw->app_window->window.boundary, (struct dpaw_point){point.x-dw->window.boundary.top_left.x,point.y-dw->window.boundary.top_left.y})
                 &&  dpaw_in_rect(dw->window.boundary, point);
    if(in_super_area || in_frame)
      match = dw;
    if(in_frame)
      break;
  }
  bool grab = false;
  if(match){
    match->drag_device = event->deviceid;
    match->drag_offset.top_left = (struct dpaw_point){point.x - match->window.boundary.top_left.x, point.y - match->window.boundary.top_left.y};
    match->drag_offset.bottom_right.x = match->window.boundary.bottom_right.x - match->window.boundary.top_left.x - match->drag_offset.top_left.x;
    match->drag_offset.bottom_right.y = match->window.boundary.bottom_right.y - match->window.boundary.top_left.y - match->drag_offset.top_left.y;
    match->drag_action = 0;
    struct dpaw_rect dw_bounds = match->app_window->window.boundary;
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
    set_focus(match);
//    printf("XI_ButtonPress %d %lf %lf %lf %lf\n", match->drag_action, event->root_x, event->root_y, event->event_x, event->event_y);
  }
  if(grab)
    dpaw_linked_list_set(&window->drag_list, &match->drag_list_entry, window->drag_list.first);
  return grab ? EHR_OK : EHR_UNHANDLED;
}

EV_ON(workspace_desktop, XI_ButtonRelease){
//  printf("XI_ButtonRelease %lf %lf %lf %lf\n", event->root_x, event->root_y, event->event_x, event->event_y);
  struct dpawindow_desktop_window* match = 0;
  for(struct dpaw_list_entry* it=window->drag_list.first; it; it=it->next){
    struct dpawindow_desktop_window* dw = container_of(it, struct dpawindow_desktop_window, drag_list_entry);
    if(dw->drag_device == event->deviceid){
      match = dw;
      break;
    }
  }
  if(match){
    dpaw_linked_list_set(0, &match->drag_list_entry, 0);
  }
  return EHR_OK;
}

EV_ON(workspace_desktop, XI_Motion){
//  printf("XI_Motion %lf %lf %lf %lf\n", event->root_x, event->root_y, event->event_x, event->event_y);
  struct dpaw_point point = {event->event_x, event->event_y};
  struct dpawindow_desktop_window* match = 0;
  for(struct dpaw_list_entry* it=window->drag_list.first; it; it=it->next){
    struct dpawindow_desktop_window* dw = container_of(it, struct dpawindow_desktop_window, drag_list_entry);
    if(dw->drag_device == event->deviceid){
      match = dw;
      break;
    }
  }
  if(match){
    switch(match->drag_action){
      case DPAW_DW_DRAG_MIDDLE      : dpawindow_move_to(&match->window, (struct dpaw_point){
        point.x-match->drag_offset.top_left.x, point.y-match->drag_offset.top_left.y
      }); break;
      case DPAW_DW_DRAG_TOP         : dpawindow_place_window(&match->window, (struct dpaw_rect){
        .top_left.y = point.y - match->drag_offset.top_left.y,
        .top_left.x = match->window.boundary.top_left.x,
        .bottom_right = match->window.boundary.bottom_right
      }); break;
      case DPAW_DW_DRAG_LEFT        : dpawindow_place_window(&match->window, (struct dpaw_rect){
        .top_left.x = point.x - match->drag_offset.top_left.x,
        .top_left.y = match->window.boundary.top_left.y,
        .bottom_right = match->window.boundary.bottom_right
      }); break;
      case DPAW_DW_DRAG_RIGHT       : dpawindow_place_window(&match->window, (struct dpaw_rect){
        .top_left = match->window.boundary.top_left,
        .bottom_right.y = match->window.boundary.bottom_right.y,
        .bottom_right.x = point.x + match->drag_offset.bottom_right.x,
      }); break;
      case DPAW_DW_DRAG_BOTTOM      : dpawindow_place_window(&match->window, (struct dpaw_rect){
        .top_left = match->window.boundary.top_left,
        .bottom_right.x = match->window.boundary.bottom_right.x,
        .bottom_right.y = point.y + match->drag_offset.bottom_right.y,
      }); break;
      case DPAW_DW_DRAG_TOP_LEFT    : dpawindow_place_window(&match->window, (struct dpaw_rect){
        .top_left.x = point.x - match->drag_offset.top_left.x,
        .top_left.y = point.y - match->drag_offset.top_left.y,
        .bottom_right = match->window.boundary.bottom_right
      }); break;
      case DPAW_DW_DRAG_TOP_RIGHT   : dpawindow_place_window(&match->window, (struct dpaw_rect){
        .top_left.y = point.y - match->drag_offset.top_left.y,
        .top_left.x = match->window.boundary.top_left.x,
        .bottom_right.y = match->window.boundary.bottom_right.y,
        .bottom_right.x = point.x + match->drag_offset.bottom_right.x,
      }); break;
      case DPAW_DW_DRAG_BOTTOM_LEFT :dpawindow_place_window(&match->window, (struct dpaw_rect){
        .top_left.y = match->window.boundary.top_left.y,
        .top_left.x = point.x - match->drag_offset.top_left.x,
        .bottom_right.x = match->window.boundary.bottom_right.x,
        .bottom_right.y = point.y + match->drag_offset.bottom_right.y,
      }); break;
      case DPAW_DW_DRAG_BOTTOM_RIGHT: dpawindow_place_window(&match->window, (struct dpaw_rect){
        .top_left = match->window.boundary.top_left,
        .bottom_right.x = point.x + match->drag_offset.bottom_right.x,
        .bottom_right.y = point.y + match->drag_offset.bottom_right.y,
      }); break;
    }
    if(match->drag_action != DPAW_DW_DRAG_MIDDLE)
      update_frame_content_boundary(match, 1);
  }
  return EHR_OK;
}

DEFINE_DPAW_WORKSPACE( desktop,
  .init = init,
  .take_window = take_window,
  .abandon_window = abandon_window,
  .screen_make_bid = screen_make_bid,
  .screen_added = screen_added,
  .screen_changed = screen_changed,
  .screen_removed = screen_removed
)
