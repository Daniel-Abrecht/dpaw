#include <dpaw.h>
#include <xev/X.c>
#include <atom/ewmh.c>
#include <atom/misc.c>
#include <dpawindow/app.h>
#include <workspace.h>
#include <stdio.h>

DEFINE_DPAW_DERIVED_WINDOW(app)

int dpawindow_app_update_wm_state(struct dpawindow_app* app){
  unsigned count = 0;
  Atom wm_state[] = {
#define X(Y) 0,
    DPAW_APP_STATE_LIST
#undef X
  };
#define X(Y) if(app->wm_state.Y) wm_state[count++] = Y;
    DPAW_APP_STATE_LIST
#undef X
  XChangeProperty(app->window.dpaw->root.display, app->window.xwindow, _NET_WM_STATE, XA_ATOM, 32, PropModeReplace, (unsigned char*)wm_state, count);
  return 0;
}

int dpawindow_app_init(struct dpaw* dpaw, struct dpawindow_app* window, Window xwindow){
  window->window.xwindow = xwindow;
  printf("dpawindow_app_init %lx\n", xwindow);
  if(dpawindow_app_init_super(dpaw, window) != 0){
    fprintf(stderr, "dpawindow_app_init_super failed\n");
    return -1;
  }

  {
    Atom* res = 0; // An atom may not be 32bit big, so specifying 4 bytes is technically wrong, but in practice, there is no other way to do this right, X will allocate a whole atom.
    if(dpaw_get_property(&window->window, _NET_WM_WINDOW_TYPE, (size_t[]){4}, 0, (void**)&res) == -1)
      fprintf(stderr, "dpaw_get_property failed\n");
    DPAW_APP_OBSERVABLE_SET(window, type, (res && *res) ? *res : _NET_WM_WINDOW_TYPE_NORMAL);
    if(res) XFree(res);
  }
  {
    Atom* res = 0;
    if(dpaw_get_property(&window->window, ONSCREEN_KEYBOARD, (size_t[]){4}, 0, (void**)&res) == -1)
      fprintf(stderr, "dpaw_get_property failed\n");
    window->is_keyboard = res && *res;
    if(res) XFree(res);
  }
  {
    XSizeHints* size_hints = XAllocSizeHints();
    if(size_hints){
      long s = 0;
      XGetWMNormalHints(dpaw->root.display, xwindow, size_hints, &s);
      if(size_hints->width <= 0)
        size_hints->width = size_hints->base_width;
      if(size_hints->height <= 0)
        size_hints->height = size_hints->base_height;
      if(size_hints->width <= 0)
        size_hints->width = window->window.boundary.bottom_right.x - window->window.boundary.top_left.x;
      if(size_hints->height <= 0)
        size_hints->height = window->window.boundary.bottom_right.y - window->window.boundary.top_left.y;
      if(size_hints->width <= 0)
        size_hints->width = 600;
      if(size_hints->height <= 0)
        size_hints->height = 400;
      DPAW_APP_OBSERVABLE_SET(window, desired_placement, *size_hints);
      XFree(size_hints);
    }else{
      DPAW_APP_OBSERVABLE_SET(window, desired_placement, (XSizeHints){0});
    }
  }
  {
    XWMHints* wm_hints = XGetWMHints(dpaw->root.display, xwindow);
    if(wm_hints){
      DPAW_APP_OBSERVABLE_SET(window, window_hints, *wm_hints);
      XFree(wm_hints);
    }else{
      DPAW_APP_OBSERVABLE_SET(window, window_hints, (XWMHints){0});
    }
  }

//  printf("%s\n", XGetAtomName(dpaw->root.display, window->observable.type.value));
  return 0;
}

int dpawindow_app_cleanup(struct dpawindow_app* window){
  return dpawindow_app_cleanup_super(window);
}

EV_ON(app, ConfigureRequest){
  // This may also be called if event->parent == window->window.xwindow
  if(event->window == window->window.xwindow)
    return dpawindow_dispatch_event(window->workspace->window, xev);
  return EHR_UNHANDLED;
}

EV_ON(app, ClientMessage){
  char* name = XGetAtomName(window->window.dpaw->root.display, event->message_type);
  printf("app: Got client message %s\n", name);
  XFree(name);
  if(event->message_type == _NET_WM_STATE)
    puts("_NET_WM_STATE");
  if(event->message_type == _NET_MOVERESIZE_WINDOW)
    puts("_NET_MOVERESIZE_WINDOW"); // TODO: Set window.observable.desired_placement
  if(event->message_type == _NET_CLOSE_WINDOW)
    puts("_NET_CLOSE_WINDOW");
  return EHR_OK;
}
