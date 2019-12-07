#include <xev/X.c>
#include <xev/xinput2.c>
#include <atom/ewmh.c>
#include <dpawindow/workspace/handheld.h>
#include <dpawindow/root.h>
#include <dpawindow/app.h>
#include <dpawindow.h>
#include <touch_gesture_detector/sideswipe.h>
#include <dpaw.h>
#include <stdlib.h>
#include <stdio.h>

static struct dpawindow_handheld_window* lookup_xwindow(struct dpawindow_workspace_handheld* handheld_workspace, Window xwindow){
  struct dpawindow_app* app_window = dpaw_workspace_lookup_xwindow(&handheld_workspace->workspace, xwindow);
  if(!app_window || !app_window->workspace_private)
    return 0;
  return app_window->workspace_private;
}

long get_desired_window_height(struct dpawindow_handheld_window* window){
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
  return boundary;
}

static int update_window_area(struct dpawindow_handheld_window* child){
  struct dpaw_rect boundary = determine_window_position(child);
  printf("update_window_area: %lx %d %ld %ld %ld %ld\n", child->app_window->window.xwindow, child->type, boundary.top_left.x, boundary.top_left.y, boundary.bottom_right.x-boundary.top_left.x, boundary.bottom_right.y-boundary.top_left.y);
  return dpawindow_place_window(&child->app_window->window, boundary);
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
    } break;
    case DPAWINDOW_HANDHELD_NORMAL: {
      if(child->workspace->current == child)
        return 0;
      struct dpawindow_handheld_window* old = child->workspace->current;
      if(old){
        old->app_window->wm_state._NET_WM_STATE_FOCUSED = false;
        dpawindow_app_update_wm_state(old->app_window);
        dpawindow_hide(&child->workspace->current->app_window->window, true);
      }
      if(!child->handheld_entry.list)
        dpaw_linked_list_set(&child->workspace->handheld_window_list, &child->handheld_entry, 0);
      child->workspace->current = child;
      child->app_window->wm_state._NET_WM_STATE_MAXIMIZED_HORZ = true;
      child->app_window->wm_state._NET_WM_STATE_MAXIMIZED_VERT = true;
    } break;
  }
  if(update_window_size(child->workspace))
    return -1;
  if(dpawindow_hide(&child->app_window->window, false))
    return -1;
  child->app_window->wm_state._NET_WM_STATE_FOCUSED = true;
  dpawindow_app_update_wm_state(child->app_window);
  if(child->type == DPAWINDOW_HANDHELD_NORMAL){
    XSetInputFocus(child->app_window->window.dpaw->root.display, child->app_window->window.xwindow, RevertToPointerRoot, CurrentTime);
  }
  return 0;
}

static void sideswipe_handler(void* private, enum dpaw_direction direction, long diff){
  if(direction >= 2)
    diff = -diff;
  struct dpawindow_workspace_handheld* workspace = private;
  struct dpawindow_handheld_window* next_window = 0;
  if(diff > 0){
    while(diff-- > 0){
      next_window = container_of(
        workspace->current && workspace->current->handheld_entry.next
         ? workspace->current->handheld_entry.next
         : workspace->handheld_window_list.first,
        struct dpawindow_handheld_window, handheld_entry
      );
    }
  }else{
    while(diff++ < 0){
      next_window = container_of(
        workspace->current && workspace->current->handheld_entry.previous
         ? workspace->current->handheld_entry.previous
         : workspace->handheld_window_list.last,
        struct dpawindow_handheld_window, handheld_entry
      );
    }
  }
  if(next_window)
    make_current(next_window);
}

static int init(struct dpawindow_workspace_handheld* workspace){
  puts("handheld_workspace init");
  {
    struct dpaw_sideswipe_detector_params params = {
      .mask = (1<<DPAW_DIRECTION_RIGHTWARDS) | (1<<DPAW_DIRECTION_LEFTWARDS),
      .private = workspace,
      .onswipe = sideswipe_handler
    };
    workspace->sideswipe_params = params;
  }
  int ret = 0;
  ret = dpaw_sideswipe_init(&workspace->sideswipe, &workspace->sideswipe_params);
  if(ret != 0){
    fprintf(stderr, "dpaw_sideswipe_init failed\n");
    return -1;
  }
  ret = dpaw_touch_gesture_manager_init(&workspace->touch_gesture_manager);
  if(ret != 0){
    fprintf(stderr, "dpaw_touch_gesture_manager_init failed\n");
    return -1;
  }
  ret = dpaw_touch_gesture_manager_add_detector(&workspace->touch_gesture_manager, &workspace->sideswipe.detector);
  if(ret != 0){
    fprintf(stderr, "dpaw_touch_gesture_manager_add_detector failed\n");
    return -1;
  }
  return ret;
}

static void cleanup(struct dpawindow_workspace_handheld* workspace){
  puts("handheld_workspace cleanup");
  dpaw_touch_gesture_manager_cleanup(&workspace->touch_gesture_manager);
}

static int screen_added(struct dpawindow_workspace_handheld* workspace, struct dpaw_workspace_screen* screen){
  (void)screen;
  puts("handheld_workspace screen_added");
  for(struct dpaw_list_entry* it=workspace->handheld_window_list.first; it; it=it->next){
    struct dpawindow_handheld_window* handheld = container_of(it, struct dpawindow_handheld_window, handheld_entry);
    update_window_area(handheld);
  }
  return 0;
}

static int screen_changed(struct dpawindow_workspace_handheld* workspace, struct dpaw_workspace_screen* screen){
  (void)screen;
  puts("handheld_workspace screen_changed");
  for(struct dpaw_list_entry* it=workspace->workspace.window_list.first; it; it=it->next){
    struct dpawindow_handheld_window* handheld = container_of(it, struct dpawindow_handheld_window, handheld_entry);
    update_window_area(handheld);
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
    if(hw->handheld_entry.next){
      puts("next");
      make_current(container_of(hw->handheld_entry.next, struct dpawindow_handheld_window, handheld_entry));
    }else if(hw->handheld_entry.previous){
      puts("previous");
      make_current(container_of(hw->handheld_entry.previous, struct dpawindow_handheld_window, handheld_entry));
    }else{
      puts("none");
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
  }
  memset(&hw->app_window->wm_state, 0, sizeof(hw->app_window->wm_state));
  dpawindow_app_update_wm_state(hw->app_window);
  dpawindow_hide(&hw->app_window->window, true);
  if(was_shown)
    update_window_size(hw->workspace);
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
  return make_current(window);
}

static int abandon_window(struct dpawindow_app* window){
  printf("abandon_window %lx\n", window->window.xwindow);
  unshow_window(window->workspace_private);
  if(window->workspace_private)
    free(window->workspace_private);
  window->workspace_private = 0;
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
  XReparentWindow(workspace->window.dpaw->root.display, window->window.xwindow, workspace->window.xwindow, 0, 0);
  if(set_window_type(child) == -1){
    abandon_window(window);
    return -1; // TODO: Do cleanup stuff
  }
  dpawindow_set_mapping(&child->app_window->window, true);
  XRaiseWindow(child->app_window->window.dpaw->root.display, child->app_window->window.xwindow);
  return 0;
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
  printf("ConfigureRequest: %lx %u %d %d %d %d\n", event->window, child->type, changes.x, changes.y, changes.width, changes.height);
  printf("%d %d %d %d\n", event->x, event->y, event->width, event->height);
  XConfigureWindow(window->window.dpaw->root.display, event->window, event->value_mask, &changes);
  update_window_area(child);
  update_window_size(child->workspace);
  if(event->value_mask & (CWWidth|CWHeight|CWX|CWY))
    DPAW_APP_OBSERVABLE_NOTIFY(child->app_window, desired_placement);
  return EHR_OK;
}

EV_ON_TOUCH(workspace_handheld){
  if(!window->current){
    dpaw_touch_gesture_manager_reset(&window->touch_gesture_manager);
    return EHR_UNHANDLED;
  }
  return dpaw_touch_gesture_manager_dispatch_touch(&window->touch_gesture_manager, event, window->workspace.window->boundary);
}

DEFINE_DPAW_WORKSPACE( handheld,
  .init = init,
  .cleanup = cleanup,
  .take_window = take_window,
  .abandon_window = abandon_window,
  .screen_make_bid = screen_make_bid,
  .screen_added = screen_added,
  .screen_changed = screen_changed,
  .screen_removed = screen_removed
)
