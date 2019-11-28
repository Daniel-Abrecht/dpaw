#include <dpaw.h>
#include <xev/X.c>
#include <atom/ewmh.c>
#include <dpawindow/app.h>
#include <workspace.h>
#include <stdio.h>

DEFINE_DPAW_DERIVED_WINDOW(app)

int dpawindow_app_init(struct dpaw* dpaw, struct dpawindow_app* window, Window xwindow){
  window->window.xwindow = xwindow;
  printf("dpawindow_app_init %lx\n", xwindow);
  if(dpawindow_app_init_super(dpaw, window) != 0){
    fprintf(stderr, "dpawindow_app_init_super failed\n");
    return -1;
  }
  int property_list_size = 0;
  Atom* property_list = XListProperties(dpaw->root.display, xwindow, &property_list_size);
  if(property_list)
  for(int i=0; i<property_list_size; i++){
    const char* name = XGetAtomName(dpaw->root.display, property_list[i]);
    printf(" - %s\n", name);
  }
  XFree(property_list);
//  XGetWindowProperty
  return 0;
}

int dpawindow_app_cleanup(struct dpawindow_app* window){
  return dpawindow_app_cleanup_super(window);
}

EV_ON(app, ClientMessage){
  (void)window;
  (void)event;
  printf("app: Got client message %s\n", XGetAtomName(window->window.dpaw->root.display, event->message_type));
  if(event->message_type == _NET_WM_STATE)
    puts("_NET_WM_STATE");
  return EHR_OK;
}
