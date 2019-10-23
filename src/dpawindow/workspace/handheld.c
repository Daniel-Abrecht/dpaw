#include <dpawindow/workspace/handheld.h>
#include <dpawindow/root.h>
#include <dpawindow/app.h>
#include <stdlib.h>
#include <stdio.h>

static int init(struct dpawindow_workspace_handheld* workspace){
  (void)workspace;
  puts("workspace init");
  XSelectInput(
    workspace->workspace.workspace_manager->root->display,
    workspace->window.xwindow,
    SubstructureRedirectMask | SubstructureNotifyMask);
  return 0;
}

static void cleanup(struct dpawindow_workspace_handheld* workspace){
  (void)workspace;
  puts("workspace cleanup");
}

static int screen_added(struct dpawindow_workspace_handheld* workspace, struct dpawin_workspace_screen* screen){
  (void)workspace;
  (void)screen;
  puts("workspace screen_added");
  return 0;
}

static int screen_changed(struct dpawindow_workspace_handheld* workspace, struct dpawin_workspace_screen* screen){
  (void)workspace;
  (void)screen;
  puts("workspace screen_changed");
  return 0;
}

static void screen_removed(struct dpawindow_workspace_handheld* workspace, struct dpawin_workspace_screen* screen){
  (void)workspace;
  (void)screen;
  puts("workspace screen_removed");
}

static int screen_make_bid(struct dpawindow_workspace_handheld* workspace, struct dpawin_workspace_screen* screen){
  (void)workspace;
  (void)screen;
  puts("workspace screen_make_bid");
  return 0;
}

static int take_window(struct dpawindow_workspace_handheld* workspace, struct dpawindow_app* window){
  struct dpawindow_handheld_window* child = calloc(sizeof(struct dpawindow_handheld_window), 1);
  if(!child)
    return -1;
  child->app_window = window;
  child->workspace = workspace;
  window->workspace_private = child;
  XReparentWindow(workspace->workspace.workspace_manager->root->display, window->window.xwindow, workspace->window.xwindow, 0, 0);
  return 0;
}

static struct dpawindow_handheld_window* lookup_xwindow(struct dpawindow_workspace_handheld* handheld_workspace, Window xwindow){
  struct dpawindow_app* app_window = dpawin_workspace_lookup_xwindow(&handheld_workspace->workspace, xwindow);
  if(!app_window || !app_window->workspace_private)
    return 0;
  return app_window->workspace_private;
}

static struct dpawin_rect determine_window_position(struct dpawindow_handheld_window* child){
  struct dpawin_rect boundary = {
    .top_left = {0,0}
  };
  struct dpawin_rect workspace_boundary = child->workspace->workspace.boundary;
  struct dpawin_point wh = {
    .x = workspace_boundary.bottom_right.x - workspace_boundary.top_left.x,
    .y = workspace_boundary.bottom_right.y - workspace_boundary.top_left.y
  };
  boundary.bottom_right.x = boundary.top_left.x + wh.x;
  boundary.bottom_right.y = boundary.top_left.y + wh.y;
  return boundary;
}

static int update_window_area(struct dpawindow_handheld_window* child){
  struct dpawin_rect boundary = determine_window_position(child);
  XWindowChanges changes = {
    .x = boundary.top_left.x,
    .y = boundary.top_left.y,
    .width  = boundary.bottom_right.x - boundary.top_left.x,
    .height = boundary.bottom_right.y - boundary.top_left.y,
  };
  XConfigureWindow(
    child->workspace->workspace.workspace_manager->root->display,
    child->app_window->window.xwindow,
    CWX | CWY | CWWidth | CWHeight,
    &changes
  );
  return 0;
}

EV_ON(workspace_handheld, MapRequest){
  struct dpawindow_handheld_window* child = lookup_xwindow(window, event->window);
  if(!child)
    return EHR_ERROR;
  if(update_window_area(child))
    return -1;
  XMapWindow(window->workspace.workspace_manager->root->display, event->window);
  return EHR_OK;
}

EV_ON(workspace_handheld, ConfigureRequest){
  puts("ConfigureRequest");
  struct dpawindow_handheld_window* child = lookup_xwindow(window, event->window);
  if(!child)
    return EHR_ERROR;
  struct dpawin_rect boundary = determine_window_position(child);
  XWindowChanges changes = {
    .x = boundary.top_left.x,
    .y = boundary.top_left.y,
    .width  = boundary.bottom_right.x - boundary.top_left.x,
    .height = boundary.bottom_right.y - boundary.top_left.y,
    .border_width = event->border_width,
    .sibling      = event->above,
    .stack_mode   = event->detail
  };
  XConfigureWindow(window->workspace.workspace_manager->root->display, event->window, event->value_mask | CWX | CWY | CWWidth | CWHeight, &changes);
  return EHR_OK;
}


DEFINE_DPAWIN_WORKSPACE( handheld,
  .init = init,
  .cleanup = cleanup,
  .take_window = take_window, \
  .screen_make_bid = screen_make_bid,
  .screen_added = screen_added,
  .screen_changed = screen_changed,
  .screen_removed = screen_removed
)
