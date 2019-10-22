#include <dpawin.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

struct dpawin dpawin;

int dpawin_cleanup(void){
  if(dpawin.root.display)
    XCloseDisplay(dpawin.root.display);
  if(dpawin.root.window.xwindow)
    dpawindow_root_cleanup(&dpawin.root);
  return 0;
}

static bool wm_detected;

static int wm_check(Display* display, XErrorEvent* error){
  (void)display;
  if(error->error_code == BadAccess)
    wm_detected = true;
  return 0;
}

int dpawin_init(void){
  memset(&dpawin, 0, sizeof(dpawin));
  dpawin.root.display = XOpenDisplay(0);
  if(!dpawin.root.display){
    fprintf(stderr, "Failed to open X display %s\n", XDisplayName(0));
    goto error;
  }
  dpawin.root.window.xwindow = DefaultRootWindow(dpawin.root.display);
  if(!dpawin.root.window.xwindow){
    fprintf(stderr, "DefaultRootWindow failed\n");
    goto error;
  }
  if(dpawindow_root_init(&dpawin.root) == -1){
    fprintf(stderr, "dpawindow_root_init failed\n");
    goto error;
  }
  wm_detected = false;
  XSetErrorHandler(&wm_check);
  XSelectInput(
      dpawin.root.display,
      dpawin.root.window.xwindow,
      SubstructureRedirectMask | SubstructureNotifyMask);
  XSync(dpawin.root.display, false);
  if(wm_detected){
    fprintf(stderr, "Detected another window manager on display %s\n", XDisplayName(0));
    goto error;
  }
  XSetErrorHandler(&dpawin_error_handler);
  return 0;
error:
  dpawin_cleanup();
  return -1;
}

int dpawin_error_handler(Display* display, XErrorEvent* error){
  char error_message[1024];
  if(XGetErrorText(display, error->error_code, error_message, sizeof(error_message)))
    *error_message = 0;
  fprintf(stderr, "XErrorEvent(serial=%ld, request_code=%d, minor_code=%d, error_code=%d): %s\n",
    error->serial,
    error->request_code,
    error->minor_code,
    error->error_code,
    error_message
  );
  return 0;
}

int dpawin_run(void){
  while(true){
    XEvent event;
    XNextEvent(dpawin.root.display, &event);
    struct dpawindow* it;
    for(it = &dpawin.root.window; it; it=it->next)
      // Note: Unlike with any other event, xany.window is actually the parent window
      if(event.xany.window == it->xwindow)
        break;
    if(it){
      enum event_handler_result result = dpawindow_dispatch_event(it, &event);
      switch(result){
        case EHR_FATAL_ERROR: {
          fprintf(stderr, "A fatal error occured while trying to handle event %d (%s) for a %s-window.\n", event.type, dpawin_get_event_name(event.type), it->type->name);
        } return -1;
        case EHR_OK: break;
        case EHR_ERROR: {
          fprintf(stderr, "An error occured while trying to handle event %d (%s) for a %s-window.\n", event.type, dpawin_get_event_name(event.type), it->type->name);
        } break;
        case EHR_UNHANDLED: {
          fprintf(stderr, "Got unhandled event %d (%s) for a %s-window.\n", event.type, dpawin_get_event_name(event.type), it->type->name);
        } break;
      }
    }else{
      fprintf(stderr, "Got event %d (%s) for unknown window (%lu)\n", event.type, dpawin_get_event_name(event.type), event.xany.window);
    }
  }
  return 0;
}
