#include <xev/X.c>
#include <xev/xinput2.c>
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

static struct dpaw_rect determine_window_position(struct dpawindow_handheld_window* child){
  struct dpaw_rect boundary = {
    .top_left = {0,0}
  };
  struct dpaw_rect workspace_boundary = child->workspace->window.boundary;
  struct dpaw_point wh = {
    .x = workspace_boundary.bottom_right.x - workspace_boundary.top_left.x,
    .y = workspace_boundary.bottom_right.y - workspace_boundary.top_left.y
  };
  boundary.bottom_right.x = boundary.top_left.x + wh.x;
  boundary.bottom_right.y = boundary.top_left.y + wh.y;
  return boundary;
}

static int update_window_area(struct dpawindow_handheld_window* child){
  struct dpaw_rect boundary = determine_window_position(child);
  return dpawindow_place_window(&child->app_window->window, boundary);
}

static int make_current(struct dpawindow_handheld_window* child){
  printf("make_current %p -> %p\n", (void*)child->workspace->current, (void*)child);
  if(child->workspace->current == child)
    return 0;
  if(child->workspace->current)
    if(dpawindow_hide(&child->workspace->current->app_window->window, true))
      return -1;
  child->workspace->current = child;
  if(update_window_area(child))
    return -1;
  if(dpawindow_hide(&child->app_window->window, false))
    return -1;
  return 0;
}

static void sideswipe_handler(void* private, enum dpaw_direction direction, long diff){
  if(direction >= 2)
    diff = -diff;
  struct dpawindow_workspace_handheld* workspace = private;
  struct dpawindow_app* appwin = workspace->current->app_window;
  if(diff > 0){
    while(diff-- > 0){
      appwin = container_of(
        appwin->workspace_window_entry.next
         ? appwin->workspace_window_entry.next
         : workspace->workspace.window_list.first,
        struct dpawindow_app, workspace_window_entry
      );
    }
  }else{
    while(diff++ < 0){
      appwin = container_of(
        appwin->workspace_window_entry.previous
         ? appwin->workspace_window_entry.previous
         : workspace->workspace.window_list.last,
        struct dpawindow_app, workspace_window_entry
      );
    }
  }
  make_current(appwin->workspace_private);
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
  for(struct dpaw_list_entry* it=workspace->workspace.window_list.first; it; it=it->next){
    struct dpawindow_app* appwin = container_of(it, struct dpawindow_app, workspace_window_entry);
    update_window_area(appwin->workspace_private);
  }
  return 0;
}

static int screen_changed(struct dpawindow_workspace_handheld* workspace, struct dpaw_workspace_screen* screen){
  (void)workspace;
  (void)screen;
  puts("handheld_workspace screen_changed");
  for(struct dpaw_list_entry* it=workspace->workspace.window_list.first; it; it=it->next){
    struct dpawindow_app* appwin = container_of(it, struct dpawindow_app, workspace_window_entry);
    update_window_area(appwin->workspace_private);
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

static int take_window(struct dpawindow_workspace_handheld* workspace, struct dpawindow_app* window){
  printf("take_window %lx\n", window->window.xwindow);
  struct dpawindow_handheld_window* child = calloc(sizeof(struct dpawindow_handheld_window), 1);
  if(!child)
    return -1;
  child->app_window = window;
  child->workspace = workspace;
  window->workspace_private = child;
  XReparentWindow(workspace->window.dpaw->root.display, window->window.xwindow, workspace->window.xwindow, 0, 0);
  if(make_current(child))
    return -1;
  dpawindow_set_mapping(&child->app_window->window, true);
  return 0;
}

static int abandon_window(struct dpawindow_app* window){
  printf("abandon_window %lx\n", window->window.xwindow);
  if(window->workspace_window_entry.next)
    make_current(container_of(window->workspace_window_entry.next, struct dpawindow_app, workspace_window_entry)->workspace_private);
  return 0;
}

EV_ON(workspace_handheld, ConfigureRequest){
  struct dpawindow_handheld_window* child = lookup_xwindow(window, event->window);
  if(!child)
    return EHR_ERROR;
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
  XConfigureWindow(window->window.dpaw->root.display, event->window, event->value_mask | CWX | CWY | CWWidth | CWHeight, &changes);
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
