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
  for(struct dpaw_list_entry* it=desktop_workspace->workspace.window_list.first; it; it=it->next){
    struct dpawindow_app* app = container_of(it, struct dpawindow_app, workspace_window_entry);
    struct dpawindow_desktop_window* dw = app->workspace_private;
    if(!dw) continue;
    if(app->window.xwindow == xwindow)
      return dw;
    if(dw->type->lookup_is_window && dw->type->lookup_is_window(dw, xwindow))
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

  if(dpawindow_xembed_init(workspace->window.dpaw, &workspace->xe_desktop)){
    fprintf(stderr, "dpawindow_xembed_init failed\n");
    return -1;
  }
  workspace->xe_desktop.parent.exclude_from_window_list = true;

  if(dpawindow_xembed_exec(
    &workspace->xe_desktop,
    XEMBED_UNSUPPORTED_TAKE_FIRST_WINDOW,
    (const char*const[]){"matchbox-desktop","--mode=window",0},
    .keep_env = true
  )){
    fprintf(stderr, "dpawindow_xembed_exec failed\n");
    return -1;
  }
  dpaw_workspace_add_window(&workspace->workspace, &workspace->xe_desktop.parent);

  return 0;
}

static void dpawindow_workspace_desktop_cleanup(struct dpawindow_workspace_desktop* workspace){
  (void)workspace;
  puts("desktop_workspace cleanup");
  dpawindow_cleanup(&workspace->xe_desktop.window); // Technically, this is currently not necessary
}

static int screen_added(struct dpawindow_workspace_desktop* workspace, struct dpaw_workspace_screen* screen){
  (void)workspace;
  (void)screen;
  puts("desktop_workspace screen_added");
  dpawindow_place_window(&workspace->xe_desktop.parent.window, workspace->window.boundary);
  return 0;
}

static int screen_changed(struct dpawindow_workspace_desktop* workspace, struct dpaw_workspace_screen* screen){
  (void)workspace;
  (void)screen;
  puts("desktop_workspace screen_changed");
  dpawindow_place_window(&workspace->xe_desktop.parent.window, workspace->window.boundary);
  return 0;
}

static void screen_removed(struct dpawindow_workspace_desktop* workspace, struct dpaw_workspace_screen* screen){
  (void)workspace;
  (void)screen;
  puts("desktop_workspace screen_removed");
  dpawindow_place_window(&workspace->xe_desktop.parent.window, workspace->window.boundary);
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

static void abandon_window(struct dpawindow_app* window){
  printf("abandon_window %lx\n", window->window.xwindow);
  struct dpawindow_desktop_window* dw = window->workspace_private;
  dpawindow_hide(&window->window, true);
  if(dw && dw->type->cleanup) dw->type->cleanup(dw);
  XDeleteProperty(window->window.dpaw->root.display, window->window.xwindow, _NET_FRAME_EXTENTS);
  XDeleteProperty(window->window.dpaw->root.display, window->window.xwindow, _NET_WM_ALLOWED_ACTIONS);
  window->workspace_private = 0;
}

static struct dpawindow_desktop_window* allocate_desktop_window(struct dpawindow_workspace_desktop* workspace, enum e_dpawindow_desktop_window_type type, struct dpawindow_app* app){
  void* derived = 0;
  struct dpawindow_desktop_window* dw = 0;

  switch(type){
#define X(NAME) \
    case DPAW_DW_DESKTOP_ ## NAME: { \
      struct dpawindow_desktop_ ## NAME* instance = calloc(1, sizeof(*instance)); \
      if(!instance) return 0; \
      derived = instance; \
      dw = &instance->dw; \
      dw->type = &dpawindow_desktop_window_type_ ## NAME; \
    } break;
    DPAWINDOW_DESKTOP_WINDOW_TYPE_LIST
#undef X
  }

  app->workspace_private = dw;
  dw->app_window = app;
  dw->workspace = workspace;

  int ret = dw->type->init(dw);

  if(ret)
    goto error;

  dpawindow_hide(&app->window, false);
  dpawindow_set_mapping(&app->window, true);

  return dw;

error:
  free(derived);
  return 0;
}

static int take_window(struct dpawindow_workspace_desktop* workspace, struct dpawindow_app* app){
  printf("take_window %lx\n", app->window.xwindow);

  struct dpawindow_desktop_window* dw = allocate_desktop_window(workspace, DPAW_DW_DESKTOP_app_window, app);
  if(!dw)
    return -1;

  return 0;
}

EV_ON(workspace_desktop, ClientMessage){
  struct dpawindow_desktop_window* child = lookup_xwindow(window, event->window);
  if(!child)
    return EHR_UNHANDLED;
/*  if(event->message_type == WM_PROTOCOLS)
    if((Atom)event->data.l[0] == WM_DELETE_WINDOW)
      hide(child);*/
  return EHR_NEXT;
}

EV_ON(workspace_desktop, ConfigureRequest){
  struct dpawindow_desktop_window* child = lookup_xwindow(window, event->window);
  if(!child)
    return EHR_UNHANDLED;
  return EHR_OK; // We know better where to place the windows. (Note: unmapped windows don't belong to a workspace & are handled elsewhere.).
}

EV_ON(workspace_desktop, XI_ButtonPress){
  return dpaw_workspace_desktop_app_window_handle_button_press(window, event);
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
