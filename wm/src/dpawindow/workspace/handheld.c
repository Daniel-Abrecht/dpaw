#include <-dpaw/dpaw.h>
#include <-dpaw/xev/X.c>
#include <-dpaw/xev/xinput2.c>
#include <-dpaw/atom/ewmh.c>
#include <-dpaw/atom/icccm.c>
#include <-dpaw/dpawindow.h>
#include <-dpaw/dpawindow/app.h>
#include <-dpaw/dpawindow/root.h>
#include <-dpaw/dpawindow/workspace/handheld.h>
#include <-dpaw/touch_gesture_detector/sideswipe.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

static struct dpawindow_handheld_window* lookup_xwindow(struct dpawindow_workspace_handheld* handheld_workspace, Window xwindow){
  struct dpawindow_app* app_window = dpaw_workspace_lookup_xwindow(&handheld_workspace->workspace, xwindow);
  if(!app_window || !app_window->workspace_private)
    return 0;
  return app_window->workspace_private;
}

static long get_desired_window_height(struct dpawindow_handheld_window* window){
  return window->app_window->observable.desired_placement.value.height;
}

static struct dpaw_rect determine_window_position(struct dpawindow_handheld_window* child){
  long    top_dock_height = child->workspace->   top_dock ? get_desired_window_height(child->workspace->   top_dock) : 0;
  long bottom_dock_height = child->workspace->bottom_dock ? get_desired_window_height(child->workspace->bottom_dock) : 0;
  struct dpaw_rect boundary = {
    .top_left = {0, 0}
  };
  struct dpaw_rect workspace_boundary = child->workspace->window.boundary;
  struct dpaw_point wh = {
    .x = workspace_boundary.bottom_right.x - workspace_boundary.top_left.x,
    .y = workspace_boundary.bottom_right.y - workspace_boundary.top_left.y
  };
  if(child->workspace->top_dock){
    if(top_dock_height > wh.y/4)
      top_dock_height =  wh.y/4;
    if(top_dock_height < 16)
      top_dock_height = 16;
  }
  if(child->workspace->bottom_dock){
    if(bottom_dock_height > wh.y/3)
      bottom_dock_height = wh.y/3;
    if(bottom_dock_height < 16)
      bottom_dock_height = 16;
  }
  boundary.bottom_right.x = wh.x;
  if(child->type == DPAWINDOW_HANDHELD_NORMAL){
    boundary.bottom_right.y = wh.y - bottom_dock_height;
    boundary.top_left.y = top_dock_height;
  }
  if(child->type == DPAWINDOW_HANDHELD_BOTTOM_DOCK){
    boundary.top_left.y = wh.y - bottom_dock_height;
    boundary.bottom_right.y = wh.y;
  }
  if(child->type == DPAWINDOW_HANDHELD_TOP_DOCK){
    boundary.top_left.y = 0;
    boundary.bottom_right.y = top_dock_height;
  }
  if(!child->active){ // see the fake_hide function
    boundary.top_left.x     -= wh.x + 10;
    boundary.bottom_right.x -= wh.x + 10;
    boundary.top_left.y     -= wh.y + 10;
    boundary.bottom_right.y -= wh.y + 10;
  }
  return boundary;
}

static int update_window_area(struct dpawindow_handheld_window* child){
  struct dpaw_rect boundary = determine_window_position(child);
//  printf("update_window_area: %lx %d %ld %ld %ld %ld\n", child->app_window->window.xwindow, child->type, boundary.top_left.x, boundary.top_left.y, boundary.bottom_right.x-boundary.top_left.x, boundary.bottom_right.y-boundary.top_left.y);
  if(child->workspace->bottom_dock == child){
    child->workspace->keyboard_top_boundary.params.line.A = boundary.top_left;
    child->workspace->keyboard_top_boundary.params.line.B = (struct dpaw_point){ boundary.bottom_right.x, boundary.top_left.y };
  }
  return dpawindow_place_window(&child->app_window->window, boundary);
}

// Some clever applications (gimp) hide all their windows if one gets hidden (iconified).
// This is a problem, because this window manager will only show one window at a time, hiding the other ones,
// and cause endles switching between windows...
// As a workaround, let's just put the window in an unviewable area and pretend it not to be iconified.
static int fake_hide(struct dpawindow_handheld_window* window, bool inactive){
  window->active = !inactive;
  return update_window_area(window);
}

static int update_window_size(struct dpawindow_workspace_handheld* workspace){
  if(workspace->current)
    update_window_area(workspace->current);
  if(workspace->top_dock)
    update_window_area(workspace->top_dock);
  if(workspace->bottom_dock)
    update_window_area(workspace->bottom_dock);
  return 0;
}

static int unshow_window(struct dpawindow_handheld_window* hw);

static int make_current(struct dpawindow_handheld_window* child){
  printf("make_current %p -> %p\n", (void*)child->workspace->current, (void*)child);
  memset(&child->app_window->wm_state, 0, sizeof(child->app_window->wm_state));
  switch(child->type){
    case DPAWINDOW_HANDHELD_UNSET: return -1;
    case DPAWINDOW_HANDHELD_TOP_DOCK: {
      if(child->workspace->top_dock == child)
        return 0;
      if(child->workspace->top_dock)
        unshow_window(child->workspace->top_dock);
      child->workspace->top_dock = child;
    } break;
    case DPAWINDOW_HANDHELD_BOTTOM_DOCK: {
      if(child->workspace->bottom_dock == child)
        return 0;
      if(child->workspace->bottom_dock)
        unshow_window(child->workspace->bottom_dock);
      child->workspace->bottom_dock = child;
      dpaw_touch_gesture_manager_add_detector(&child->workspace->touch_gesture_manager, &child->workspace->keyboard_top_boundary.detector);
    } break;
    case DPAWINDOW_HANDHELD_NORMAL: {
      if(child->workspace->current == child)
        return 0;
      if(child->workspace->current && child->workspace->current->handheld_entry.list) // Only go back to windows in the list of regular windows
        child->workspace->previous = child->workspace->current;
      if(child->workspace->previous == child)
        child->workspace->previous = 0; // The new current window was the previous window, the window shown before that is unknown
      struct dpawindow_handheld_window* old = child->workspace->current;
      if(old){
        old->app_window->wm_state._NET_WM_STATE_FOCUSED = false;
        dpawindow_app_update_wm_state(old->app_window);
        fake_hide(child->workspace->current, true);
      }
      child->workspace->current = child;
      child->app_window->wm_state._NET_WM_STATE_MAXIMIZED_HORZ = true;
      child->app_window->wm_state._NET_WM_STATE_MAXIMIZED_VERT = true;
    } break;
  }
  if(update_window_size(child->workspace))
    return -1;
  if(fake_hide(child, false))
    return -1;
  child->app_window->wm_state._NET_WM_STATE_FOCUSED = true;
  dpawindow_app_update_wm_state(child->app_window);
  if(child->type == DPAWINDOW_HANDHELD_NORMAL){
    XRaiseWindow(child->app_window->window.dpaw->root.display, child->app_window->window.xwindow);
    dpaw_workspace_set_focus(child->app_window);
  }
  return 0;
}

static void show_next_window(struct dpawindow_workspace_handheld* workspace){
  struct dpawindow_handheld_window* window = container_of(
    workspace->current && workspace->current->handheld_entry.next
      ? workspace->current->handheld_entry.next
      : workspace->handheld_window_list.first,
    struct dpawindow_handheld_window, handheld_entry
  );
  if(window)
    make_current(window);
}

static void show_previous_window(struct dpawindow_workspace_handheld* workspace){
  struct dpawindow_handheld_window* window = container_of(
    workspace->current && workspace->current->handheld_entry.previous
      ? workspace->current->handheld_entry.previous
      : workspace->handheld_window_list.last,
    struct dpawindow_handheld_window, handheld_entry
  );
  if(window)
    make_current(window);
}

static void sideswipe_handler(void *private, struct dpaw_touch_gesture_detector* detector){
  struct dpaw_sideswipe_detector* sideswipe = container_of(detector, struct dpaw_sideswipe_detector, detector);
  struct dpawindow_workspace_handheld* workspace = private;
  int x_third = (sideswipe->initial_position.x - workspace->window.boundary.top_left.x) * 3 / (workspace->window.boundary.bottom_right.x - workspace->window.boundary.top_left.x);
  bool bottom_half = sideswipe->initial_position.y > (workspace->window.boundary.bottom_right.y + workspace->window.boundary.top_left.y) / 2;
  switch(sideswipe->direction){
    case DPAW_DIRECTION_UPWARDS: make_current(workspace->dashboard.parent.workspace_private); break;
    case DPAW_DIRECTION_DOWNWARDS: {
      switch(x_third){
        case 0: break;
        case 1: dpawindow_close(&workspace->current->app_window->window); break;
        case 2: break;
      }
    } break;
    case DPAW_DIRECTION_RIGHTWARDS: {
      if(workspace->current && !workspace->current->handheld_entry.list)
        break;
      if(bottom_half){
        show_previous_window(workspace);
      }else{
        show_next_window(workspace);
      }
    }; break;
    case DPAW_DIRECTION_LEFTWARDS: {
      if(workspace->current && !workspace->current->handheld_entry.list)
        break;
      if(bottom_half){
        show_next_window(workspace);
      }else{
        show_previous_window(workspace);
      }
    } break;
  }
}

static enum event_handler_result bottom_dock_top_line_resize_handler(
  void* private,
  struct dpaw_touch_gesture_detector* detector,
  struct dpaw_touch_event* te
){
  struct dpawindow_workspace_handheld* workspace = private;
  if(!workspace->bottom_dock)
    return EHR_UNHANDLED;
  (void)detector;
  long height = workspace->window.boundary.bottom_right.y - (long)te->event.root_y;
  workspace->bottom_dock->app_window->observable.desired_placement.value.height = height;
  DPAW_APP_OBSERVABLE_NOTIFY(workspace->bottom_dock->app_window, desired_placement);
  return EHR_NEXT;
}

static int init(struct dpawindow_workspace_handheld* workspace){
  puts("handheld_workspace init");
  int ret = 0;

  workspace->window.type = &dpawindow_type_workspace_handheld;
  workspace->window.dpaw = workspace->workspace.workspace_manager->dpaw;
  if(dpawindow_register(&workspace->window)){
    fprintf(stderr, "dpawindow_register failed\n");
    return -1;
  }

/*  if(dpawindow_xembed_init(workspace->window.dpaw, &workspace->keyboard)){
    fprintf(stderr, "dpawindow_xembed_init failed\n");
    return -1;
  }*/

  if(dpawindow_xembed_init(workspace->window.dpaw, &workspace->dashboard)){
    fprintf(stderr, "dpawindow_xembed_init failed\n");
    return -1;
  }
  workspace->dashboard.parent.exclude_from_window_list = true;

  struct dpaw_sideswipe_detector_params sideswipe_params = {
    .mask = (1<<DPAW_DIRECTION_RIGHTWARDS)
          | (1<<DPAW_DIRECTION_LEFTWARDS)
          | (1<<DPAW_DIRECTION_DOWNWARDS)
          | (1<<DPAW_DIRECTION_UPWARDS),
    .bounds = &workspace->window.boundary
  };
  ret = dpaw_sideswipe_init(&workspace->sideswipe, workspace->window.dpaw, &sideswipe_params);
  if(ret != 0){
    fprintf(stderr, "dpaw_sideswipe_init failed\n");
    return -1;
  }
  workspace->sideswipe.detector.private = workspace;
  workspace->sideswipe.detector.ongesture = sideswipe_handler;

  struct dpaw_line_touch_detector_params keyboard_top_boundary_params;
  memset(&keyboard_top_boundary_params, 0, sizeof(keyboard_top_boundary_params));
  ret = dpaw_line_touch_init(&workspace->keyboard_top_boundary, &keyboard_top_boundary_params, workspace->window.dpaw);
  if(ret != 0){
    fprintf(stderr, "dpaw_sideswipe_init failed\n");
    return -1;
  }
  workspace->keyboard_top_boundary.detector.private = workspace;
  workspace->keyboard_top_boundary.detector.ontouch = bottom_dock_top_line_resize_handler;

  ret = dpaw_touch_gesture_manager_init(&workspace->touch_gesture_manager, workspace->window.dpaw);
  if(ret != 0){
    fprintf(stderr, "dpaw_touch_gesture_manager_init failed\n");
    return -1;
  }

  ret = dpaw_touch_gesture_manager_add_detector(&workspace->touch_gesture_manager, &workspace->sideswipe.detector);
  if(ret != 0){
    fprintf(stderr, "dpaw_touch_gesture_manager_add_detector failed\n");
    return -1;
  }

/*  if(dpawindow_xembed_exec(
    &workspace->keyboard,
    XEMBED_METHOD_TAKE_WINDOW_FROM_STDOUT,
    (const char*const[]){"onboard","--xid",0},
    .keep_env = true
  )){
    fprintf(stderr, "dpawindow_xembed_exec failed\n");
    return -1;
  }
  workspace->keyboard.parent.is_keyboard = true;
  dpaw_workspace_add_window(&workspace->workspace, &workspace->keyboard.parent);*/

  if(dpawindow_xembed_exec(
    &workspace->dashboard,
    XEMBED_METHOD_GIVE_WINDOW_BY_ARGUMENT,
    (const char*const[]){"dpaw-dashboard","--into","<XID>",0},
    .keep_env = true
  )){
    fprintf(stderr, "dpawindow_xembed_exec failed\n");
    return -1;
  }
  dpaw_workspace_add_window(&workspace->workspace, &workspace->dashboard.parent);

  return ret;
}

static void dpawindow_workspace_handheld_cleanup(struct dpawindow_workspace_handheld* workspace){
  puts("handheld_workspace cleanup");
  dpaw_touch_gesture_manager_cleanup(&workspace->touch_gesture_manager);
  dpawindow_cleanup(&workspace->keyboard.window); // Technically, this is currently not necessary
  dpawindow_cleanup(&workspace->dashboard.window); // Technically, this is currently not necessary
}

static int screen_added(struct dpawindow_workspace_handheld* workspace, struct dpaw_workspace_screen* screen){
  (void)screen;
  puts("handheld_workspace screen_added");
  for(struct dpaw_list_entry* it=workspace->workspace.window_list.first; it; it=it->next){
    struct dpawindow_app* app = container_of(it, struct dpawindow_app, workspace_window_entry);
    update_window_area(app->workspace_private);
  }
  return 0;
}

static int screen_changed(struct dpawindow_workspace_handheld* workspace, struct dpaw_workspace_screen* screen){
  (void)screen;
  puts("handheld_workspace screen_changed");
  for(struct dpaw_list_entry* it=workspace->workspace.window_list.first; it; it=it->next){
    struct dpawindow_app* app = container_of(it, struct dpawindow_app, workspace_window_entry);
    update_window_area(app->workspace_private);
  }
  return 0;
}

static void screen_removed(struct dpawindow_workspace_handheld* workspace, struct dpaw_workspace_screen* screen){
  (void)workspace;
  (void)screen;
  puts("handheld_workspace screen_removed");
}

static int screen_make_bid(struct dpawindow_workspace_handheld* workspace, struct dpaw_workspace_screen* screen){
  (void)workspace;
  (void)screen;
  puts("handheld_workspace screen_make_bid");
  return 0;
}

static int unshow_window(struct dpawindow_handheld_window* hw){
  if(!hw)
    return 0;
  printf("unshow_window %lx\n", hw->app_window->window.xwindow);
  bool was_shown = false;
  if(hw == hw->workspace->current){
    was_shown = true;
    if(hw->workspace->previous){
      assert(hw->workspace->previous->handheld_entry.list); // previous window must be in regular workspace window list
      make_current(hw->workspace->previous);
    }else if(hw->handheld_entry.next){
      make_current(container_of(hw->handheld_entry.next, struct dpawindow_handheld_window, handheld_entry));
    }else if(hw->handheld_entry.previous){
      make_current(container_of(hw->handheld_entry.previous, struct dpawindow_handheld_window, handheld_entry));
    }else if(&hw->workspace->dashboard.parent != hw->app_window){
      // Nothing else there, show dashboard
      make_current(hw->workspace->dashboard.parent.workspace_private);
    }else{
      hw->workspace->current = 0;
    }
  }
  dpaw_linked_list_set(0, &hw->handheld_entry, 0);
  if(hw == hw->workspace->top_dock){
    was_shown = true;
    hw->workspace->top_dock = 0;
  }
  if(hw == hw->workspace->bottom_dock){
    was_shown = true;
    hw->workspace->bottom_dock = 0;
    dpaw_touch_gesture_manager_remove_detector(&hw->workspace->keyboard_top_boundary.detector);
  }
  memset(&hw->app_window->wm_state, 0, sizeof(hw->app_window->wm_state));
  dpawindow_app_update_wm_state(hw->app_window);
  fake_hide(hw, true);
  if(was_shown)
    update_window_size(hw->workspace);
  if(hw == hw->workspace->previous)
    hw->workspace->previous = 0;
  return 0;
}

static int set_window_type(struct dpawindow_handheld_window* window){
  enum dpawindow_handheld_window_type type = DPAWINDOW_HANDHELD_NORMAL;
  if((!window->workspace->top_dock || window->workspace->top_dock == window) && (
      window->app_window->observable.type.value == _NET_WM_WINDOW_TYPE_TOOLBAR
   || window->app_window->observable.type.value == _NET_WM_WINDOW_TYPE_MENU
  )){
    type = DPAWINDOW_HANDHELD_TOP_DOCK;
  }
  if(window->app_window->observable.type.value == _NET_WM_WINDOW_TYPE_DOCK){
    if(window->workspace->top_dock == window){
      type = DPAWINDOW_HANDHELD_TOP_DOCK;
    }else if(window->workspace->bottom_dock == window){
      type = DPAWINDOW_HANDHELD_BOTTOM_DOCK;
    }else if(!window->workspace->top_dock && !window->workspace->bottom_dock){
      long workspace_height = window->workspace->window.boundary.bottom_right.y - window->workspace->window.boundary.top_left.y;
      type = window->app_window->window.boundary.top_left.y < workspace_height / 2 ? DPAWINDOW_HANDHELD_TOP_DOCK : DPAWINDOW_HANDHELD_BOTTOM_DOCK;
    }else if(!window->workspace->top_dock){
      type = DPAWINDOW_HANDHELD_TOP_DOCK;
    }else if(!window->workspace->bottom_dock){
      type = DPAWINDOW_HANDHELD_BOTTOM_DOCK;
    }
  }
  if( window->app_window->is_keyboard && (
        !window->workspace->bottom_dock
     ||  window->workspace->bottom_dock == window
     || !window->workspace->bottom_dock->app_window->is_keyboard
  )) type = DPAWINDOW_HANDHELD_BOTTOM_DOCK;
  if(window->type == type)
    return 0;
  if(window->type != DPAWINDOW_HANDHELD_UNSET)
    unshow_window(window);
  window->type = type;
  if(type == DPAWINDOW_HANDHELD_NORMAL && !window->app_window->exclude_from_window_list){
    if(!window->handheld_entry.list)
      dpaw_linked_list_set(&window->workspace->handheld_window_list, &window->handheld_entry, 0);
  }else{
    dpaw_linked_list_set(0, &window->handheld_entry, 0);
    if(window->workspace->previous == window)
      window->workspace->previous = 0;
  }
  return make_current(window);
}

static void abandon_window(struct dpawindow_app* window){
  printf("abandon_window %lx\n", window->window.xwindow);
  dpawindow_hide(&window->window, true);
  unshow_window(window->workspace_private);
  XDeleteProperty(window->window.dpaw->root.display, window->window.xwindow, _NET_FRAME_EXTENTS);
  XDeleteProperty(window->window.dpaw->root.display, window->window.xwindow, _NET_WM_ALLOWED_ACTIONS);
  if(window->workspace_private)
    free(window->workspace_private);
  window->workspace_private = 0;
}

int desired_placement_change_handler(void* private, struct dpawindow_app* app, XSizeHints size){
  (void)private;
  (void)size;
  struct dpawindow_handheld_window* handheld_window = app->workspace_private;
  printf("desired_placement_change_handler %ld %dx%d\n", app->window.xwindow, app->observable.desired_placement.value.width, app->observable.desired_placement.value.height);
  update_window_area(handheld_window);
  update_window_size(handheld_window->workspace);
  return 0;
}

static int take_window(struct dpawindow_workspace_handheld* workspace, struct dpawindow_app* window){
  printf("take_window %lx\n", window->window.xwindow);
  struct dpawindow_handheld_window* child = calloc(sizeof(struct dpawindow_handheld_window), 1);
  if(!child)
    return -1;
  child->app_window = window;
  child->workspace = workspace;
  window->workspace_private = child;
  XChangeProperty(child->app_window->window.dpaw->root.display, child->app_window->window.xwindow, _NET_FRAME_EXTENTS, XA_CARDINAL, 32, PropModeReplace, (unsigned char*)(long[]){0,0,0,0}, 4);
  XChangeProperty(child->app_window->window.dpaw->root.display, child->app_window->window.xwindow, _NET_WM_ALLOWED_ACTIONS, XA_ATOM, 32, PropModeReplace, (unsigned char*)(Atom[]){_NET_WM_ACTION_CLOSE}, 1);
  XReparentWindow(workspace->window.dpaw->root.display, window->window.xwindow, workspace->window.xwindow, 0, 0);
  if(set_window_type(child) == -1){
    abandon_window(window);
    return -1; // TODO: Do cleanup stuff
  }
  printf("take_window: %lx %u %d %d\n", child->app_window->window.xwindow, child->type, child->app_window->observable.desired_placement.value.width, child->app_window->observable.desired_placement.value.height);
  DPAW_APP_OBSERVE(child->app_window, desired_placement, 0, desired_placement_change_handler);
  dpawindow_hide(&window->window, false);
  dpawindow_set_mapping(&child->app_window->window, true);
  XRaiseWindow(child->app_window->window.dpaw->root.display, child->app_window->window.xwindow);
  return 0;
}

EV_ON(workspace_handheld, ClientMessage){
  struct dpawindow_handheld_window* child = lookup_xwindow(window, event->window);
  if(!child)
    return EHR_UNHANDLED;
  if(event->message_type == WM_PROTOCOLS){
    if((Atom)event->data.l[0] == WM_DELETE_WINDOW){
      if(window->handheld_window_list.first)
        unshow_window(child);
    }
  }
  return EHR_NEXT;
}

EV_ON(workspace_handheld, ConfigureRequest){
  struct dpawindow_handheld_window* child = lookup_xwindow(window, event->window);
  if(!child)
    return EHR_UNHANDLED;
  if(event->value_mask & CWWidth)
    child->app_window->observable.desired_placement.value.width = event->width;
  if(event->value_mask & CWHeight)
    child->app_window->observable.desired_placement.value.height = event->height;
  if(event->value_mask & CWX)
    child->app_window->observable.desired_placement.value.x = event->x;
  if(event->value_mask & CWY)
    child->app_window->observable.desired_placement.value.y = event->y;
  struct dpaw_rect boundary = determine_window_position(child);
  XWindowChanges changes = {
    .x = boundary.top_left.x,
    .y = boundary.top_left.y,
    .width  = boundary.bottom_right.x - boundary.top_left.x,
    .height = boundary.bottom_right.y - boundary.top_left.y,
    .border_width = event->border_width,
    .sibling      = event->above,
    .stack_mode   = event->detail
  };
//  printf("ConfigureRequest: %lx %u %d %d %d %d\n", event->window, child->type, changes.x, changes.y, changes.width, changes.height);
//  printf("%d %d %d %d\n", event->x, event->y, event->width, event->height);
//  printf("%d %d %d %d\n", child->app_window->observable.desired_placement.value.x, child->app_window->observable.desired_placement.value.y, child->app_window->observable.desired_placement.value.width, child->app_window->observable.desired_placement.value.height);
  XConfigureWindow(window->window.dpaw->root.display, event->window, event->value_mask, &changes);
  if(event->value_mask & (CWWidth|CWHeight|CWX|CWY))
    DPAW_APP_OBSERVABLE_NOTIFY(child->app_window, desired_placement);
  return EHR_OK;
}

EV_ON_TOUCH(workspace_handheld){
  if(!window->current){
    dpaw_touch_gesture_manager_reset(&window->touch_gesture_manager);
    return EHR_UNHANDLED;
  }
  return dpaw_touch_gesture_manager_dispatch_touch(&window->touch_gesture_manager, event);
}

DEFINE_DPAW_WORKSPACE( handheld,
  .init = init,
  .take_window = take_window,
  .abandon_window = abandon_window,
  .screen_make_bid = screen_make_bid,
  .screen_added = screen_added,
  .screen_changed = screen_changed,
  .screen_removed = screen_removed
)
