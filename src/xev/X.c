#include <xev/X.c>
#include <dpawin.h>
#include <dpawindow.h>

int dpawin_xev_X_init(struct dpawin* dpawin, struct dpawin_xev* xev){
  (void)dpawin;
  // The standard X event aren't extensions, so let's use an invalid extension opcode as a sentinel.
  xev->extension = -1;
  return 0;
}

int dpawin_xev_X_cleanup(struct dpawin* dpawin, struct dpawin_xev* xev){
  (void)xev;
  (void)dpawin;
  return 0;
}

enum event_handler_result dpawin_xev_X_dispatch(struct dpawin* dpawin, struct dpawin_xev* xev, int event, void* data){
  if(event == GenericEvent || event == KeyPress || event == KeyRelease)
    return EHR_UNHANDLED;
  XEvent* ev = data;
  struct dpawindow* it = 0;
  for(it = &dpawin->root.window; it; it=it->next)
    if(ev->xany.window == it->xwindow)
      break;
  if(!it || it == &dpawin->root.window)
    return EHR_UNHANDLED;
  return dpawindow_dispatch_event(it, xev->xev, event, data);
}
