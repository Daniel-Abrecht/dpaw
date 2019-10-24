#include <dpawindow/app.h>
#include <workspace.h>
#include <stdio.h>

DEFINE_DPAWIN_DERIVED_WINDOW(app)

int dpawindow_app_init(struct dpawin* dpawin, struct dpawindow_app* window, Window xwindow){
  window->window.xwindow = xwindow;
  if(dpawindow_app_init_super(dpawin, window) != 0){
    fprintf(stderr, "dpawindow_app_init_super failed\n");
    return -1;
  }
  return 0;
}

int dpawindow_app_cleanup(struct dpawindow_app* window){
  (void)window;
  return 0;
}
