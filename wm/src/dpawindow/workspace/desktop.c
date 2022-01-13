#include <-dpaw/dpaw.h>
#include <-dpaw/xev/X.c>
#include <-dpaw/xev/xinput2.c>
#include <-dpaw/atom/ewmh.c>
#include <-dpaw/atom/icccm.c>
#include <-dpaw/dpawindow.h>
#include <-dpaw/dpawindow/app.h>
#include <-dpaw/dpawindow/root.h>
#include <-dpaw/dpawindow/workspace/desktop.h>
#include <-dpaw/touch_gesture_detector/sideswipe.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

static struct dpawindow_desktop_window* lookup_xwindow(struct dpawindow_workspace_desktop* desktop_workspace, Window xwindow){
  struct dpawindow_app* app_window = dpaw_workspace_lookup_xwindow(&desktop_workspace->workspace, xwindow);
  if(!app_window || !app_window->workspace_private)
    return 0;
  return app_window->workspace_private;
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
  return 0;
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

static int take_window(struct dpawindow_workspace_desktop* workspace, struct dpawindow_app* window){
  printf("take_window %lx\n", window->window.xwindow);
  struct dpawindow_desktop_window* child = calloc(sizeof(struct dpawindow_desktop_window), 1);
  if(!child)
    return -1;
  child->app_window = window;
  child->workspace = workspace;
  window->workspace_private = child;
  XChangeProperty(child->app_window->window.dpaw->root.display, child->app_window->window.xwindow, _NET_FRAME_EXTENTS, XA_CARDINAL, 32, PropModeReplace, (unsigned char*)(long[]){0,0,0,0}, 4);
  XChangeProperty(child->app_window->window.dpaw->root.display, child->app_window->window.xwindow, _NET_WM_ALLOWED_ACTIONS, XA_ATOM, 32, PropModeReplace, (unsigned char*)(Atom[]){_NET_WM_ACTION_CLOSE}, 1);
  XReparentWindow(workspace->window.dpaw->root.display, window->window.xwindow, workspace->window.xwindow, 0, 0);

  // TODO: update position & stuff for child

  printf("take_window: %lx %d %d\n", child->app_window->window.xwindow, child->app_window->observable.desired_placement.value.width, child->app_window->observable.desired_placement.value.height);
  DPAW_APP_OBSERVE(child->app_window, desired_placement, 0, desired_placement_change_handler);
  dpawindow_hide(&window->window, false);
  dpawindow_set_mapping(&child->app_window->window, true);
  XRaiseWindow(child->app_window->window.dpaw->root.display, child->app_window->window.xwindow);
  return 0;
}

EV_ON(workspace_desktop, ClientMessage){
  struct dpawindow_desktop_window* child = lookup_xwindow(window, event->window);
  if(!child)
    return EHR_UNHANDLED;
  if(event->message_type == WM_PROTOCOLS){
    if((Atom)event->data.l[0] == WM_DELETE_WINDOW){
      if(window->desktop_window_list.first)
        unshow_window(child);
    }
  }
  return EHR_NEXT;
}

/*EV_ON(workspace_desktop, ConfigureRequest){
  struct dpawindow_desktop_window* child = lookup_xwindow(window, event->window);
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
}*/

DEFINE_DPAW_WORKSPACE( desktop,
  .init = init,
  .take_window = take_window,
  .abandon_window = abandon_window,
  .screen_make_bid = screen_make_bid,
  .screen_added = screen_added,
  .screen_changed = screen_changed,
  .screen_removed = screen_removed
)
