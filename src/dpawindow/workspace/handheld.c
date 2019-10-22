#include <dpawindow/workspace/handheld.h>
#include <dpawindow/root.h>
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
  return 1;
}

static int take_window(struct dpawindow_workspace_handheld* workspace, Window window){
  XReparentWindow(workspace->workspace.workspace_manager->root->display, window, workspace->window.xwindow, 0, 0);
  return 0;
}


EV_ON(workspace_handheld, MapRequest){
  XMapWindow(window->workspace.workspace_manager->root->display, event->window);
  return EHR_OK;
}

EV_ON(workspace_handheld, ConfigureRequest){
  puts("ConfigureRequest");
  XWindowChanges changes = {
    .x = event->x,
    .y = event->y,
    .width  = event->width,
    .height = event->height,
    .border_width = event->border_width,
    .sibling      = event->above,
    .stack_mode   = event->detail
  };
  XConfigureWindow(window->workspace.workspace_manager->root->display, event->window, event->value_mask, &changes);
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
