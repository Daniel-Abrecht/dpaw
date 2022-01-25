#include <-dpaw/dpaw.h>
#include <-dpaw/xev/X.c>
#include <-dpaw/atom/ewmh.c>
#include <-dpaw/atom/misc.c>
#include <-dpaw/dpawindow/app.h>
#include <-dpaw/workspace.h>
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

  window->window.type = &dpawindow_type_app;
  window->window.dpaw = dpaw;
  if(dpawindow_register(&window->window)){
    fprintf(stderr, "dpawindow_register failed\n");
    return -1;
  }

  {
    Atom* res = 0; // An atom may not be 32bit big, so specifying 4 bytes is technically wrong, but in practice, there is no other way to do this right, X will allocate a whole atom.
    if(dpaw_get_property(&window->window, _NET_WM_WINDOW_TYPE, (size_t[]){4}, 0, (void**)&res) == -1)
      fprintf(stderr, "dpaw_get_property(_NET_WM_WINDOW_TYPE) failed\n");
    DPAW_APP_OBSERVABLE_SET(window, type, (res && *res) ? *res : _NET_WM_WINDOW_TYPE_NORMAL);
    if(res) XFree(res);
  }
  {
    Atom* res = 0;
    if(dpaw_get_property(&window->window, ONSCREEN_KEYBOARD, (size_t[]){4}, 0, (void**)&res) == -1)
      fprintf(stderr, "dpaw_get_property(ONSCREEN_KEYBOARD) failed\n");
    window->is_keyboard = res && *res;
    if(res) XFree(res);
  }
  if(!dpawindow_get_reasonable_size_hints(&window->window, &window->observable.desired_placement.value)){
    DPAW_APP_OBSERVABLE_NOTIFY(window, desired_placement);
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

  {
    struct dpaw_string string = {0};
    if(dpaw_get_property(&window->window, _NET_WM_NAME, &string.size, 0, (void**)&string.data) == -1)
    if(dpaw_get_property(&window->window, XA_WM_NAME, &string.size, 0, (void**)&string.data) == -1)
      fprintf(stderr, "dpaw_get_property(_NET_WM_NAME) failed\n");
    DPAW_APP_OBSERVABLE_SET(window, name, string);
  }

//  printf("%s\n", XGetAtomName(dpaw->root.display, window->observable.type.value));
  return 0;
}

EV_ON(app, PropertyNotify){
  if(event->atom == _NET_WM_NAME || event->atom == XA_WM_NAME){
    struct dpaw_string string = {0};
    if(dpaw_get_property(&window->window, _NET_WM_NAME, &string.size, 0, (void**)&string.data) == -1)
    if(dpaw_get_property(&window->window, XA_WM_NAME, &string.size, 0, (void**)&string.data) == -1)
      fprintf(stderr, "dpaw_get_property(_NET_WM_NAME) failed\n");
    DPAW_APP_OBSERVABLE_SET(window, name, string);
  }else if(event->atom == _NET_WM_WINDOW_TYPE){
    Atom* res = 0; // An atom may not be 32bit big, so specifying 4 bytes is technically wrong, but in practice, there is no other way to do this right, X will allocate a whole atom.
    if(dpaw_get_property(&window->window, _NET_WM_WINDOW_TYPE, (size_t[]){4}, 0, (void**)&res) == -1){
      fprintf(stderr, "dpaw_get_property(_NET_WM_WINDOW_TYPE) failed\n");
    }else{
      DPAW_APP_OBSERVABLE_SET(window, type, (res && *res) ? *res : _NET_WM_WINDOW_TYPE_NORMAL);
    }
    if(res) XFree(res);
  }else return EHR_UNHANDLED;
  return EHR_OK;
}

static void dpawindow_app_cleanup(struct dpawindow_app* app){
  XReparentWindow(app->window.dpaw->root.display, app->window.xwindow, app->window.dpaw->root.window.xwindow, 0, 0);
  if(app->workspace)
    dpaw_workspace_remove_window(app);
  XRemoveFromSaveSet(app->window.dpaw->root.display, app->window.xwindow);
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
  if(name)
    XFree(name);
  if(event->message_type == _NET_WM_STATE){
    char* sn1 = 0;
    char* sn2 = 0;
    if(event->data.l[1])
      sn1 = XGetAtomName(window->window.dpaw->root.display, event->data.l[1]);
    if(event->data.l[2])
      sn2 = XGetAtomName(window->window.dpaw->root.display, event->data.l[2]);
    printf("_NET_WM_STATE action: %ld %s %s si: %ld\n", event->data.l[0], sn1, sn2, event->data.l[3]);
    if(sn1) XFree(sn1);
    if(sn2) XFree(sn2);
  }
  if(event->message_type == _NET_MOVERESIZE_WINDOW)
    puts("_NET_MOVERESIZE_WINDOW"); // TODO: Set window.observable.desired_placement
  if(event->message_type == _NET_CLOSE_WINDOW)
    puts("_NET_CLOSE_WINDOW");
  return EHR_OK;
}

EV_ON(app, MapRequest){
  if(window->window.xwindow == event->window)
    return EHR_NEXT; // It's me

  struct dpawindow* win = dpawindow_lookup(window->window.dpaw, event->window);
  if(win) // This is one of our windows
    return EHR_NEXT;

  if(window->got_foreign_window && !window->got_foreign_window(window, event->window))
    return EHR_OK;

  return EHR_NEXT;
}

EV_ON(app, ReparentNotify){
  printf("app ReparentNotify %lx\n", event->window);

  if(window->window.xwindow == event->window)
    return EHR_NEXT; // It's me

  struct dpawindow* win = dpawindow_lookup(window->window.dpaw, event->window);
  if(win) // This is one of our windows
    return EHR_NEXT;

  if(window->got_foreign_window && !window->got_foreign_window(window, event->window))
    return EHR_OK;

  return EHR_NEXT;
}
