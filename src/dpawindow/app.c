#include <dpaw.h>
#include <xev/X.c>
#include <atom/ewmh.c>
#include <dpawindow/app.h>
#include <workspace.h>
#include <stdint.h>
#include <stdio.h>

DEFINE_DPAW_DERIVED_WINDOW(app)

int dpawindow_app_init(struct dpaw* dpaw, struct dpawindow_app* window, Window xwindow){
  window->window.xwindow = xwindow;
  printf("dpawindow_app_init %lx\n", xwindow);
  if(dpawindow_app_init_super(dpaw, window) != 0){
    fprintf(stderr, "dpawindow_app_init_super failed\n");
    return -1;
  }
/*  int property_list_size = 0;
  Atom* property_list = XListProperties(dpaw->root.display, xwindow, &property_list_size);
  if(property_list)
  for(int i=0; i<property_list_size; i++){
    char* name = XGetAtomName(dpaw->root.display, property_list[i]);
    printf(" - %s\n", name);
    XFree(name);
  }
  XFree(property_list);*/
  {
    uint32_t* res = 0; // Note: The Atom type may not be 32 bytes big!!!
    if(dpaw_get_property(&window->window, _NET_WM_WINDOW_TYPE, (size_t[]){4}, 0, (void**)&res) == -1)
      fprintf(stderr, "dpaw_get_property failed\n");
    DPAW_APP_OBSERVABLE_SET(window, type, (res && *res) ? *res : _NET_WM_WINDOW_TYPE_NORMAL);
    if(res) XFree(res);
  }
  {
    XSizeHints* size_hints = XAllocSizeHints();
    if(size_hints){
      long s = 0;
      XGetWMNormalHints(dpaw->root.display, xwindow, size_hints, &s);
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
  printf("%s\n", XGetAtomName(dpaw->root.display, window->observable.type.value));
  return 0;
}

int dpawindow_app_cleanup(struct dpawindow_app* window){
  return dpawindow_app_cleanup_super(window);
}

EV_ON(app, ClientMessage){
  (void)window;
  (void)event;
  char* name = XGetAtomName(window->window.dpaw->root.display, event->message_type);
  printf("app: Got client message %s\n", name);
  XFree(name);
  if(event->message_type == _NET_WM_STATE)
    puts("_NET_WM_STATE");
  return EHR_OK;
}
