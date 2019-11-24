#include <dpawindow/app.h>
#include <workspace.h>
#include <stdio.h>

DEFINE_DPAW_DERIVED_WINDOW(app)

int dpawindow_app_init(struct dpaw* dpaw, struct dpawindow_app* window, Window xwindow){
  window->window.xwindow = xwindow;
  if(dpawindow_app_init_super(dpaw, window) != 0){
    fprintf(stderr, "dpawindow_app_init_super failed\n");
    return -1;
  }
  return 0;
}

int dpawindow_app_cleanup(struct dpawindow_app* window){
  return dpawindow_app_cleanup_super(window);
}
