#include <dpawin.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

int dpawin_cleanup(struct dpawin* dpawin){
  if(dpawin->root.display)
    XCloseDisplay(dpawin->root.display);
  return 0;
}

static bool wm_detected;

static int wm_check(Display* display, XErrorEvent* error){
  (void)display;
  if(error->error_code == BadAccess)
    wm_detected = true;
  return 0;
}

int dpawin_init(struct dpawin* dpawin){
  memset(dpawin, 0, sizeof(*dpawin));
  dpawindow_root_init_super(&dpawin->root);
  dpawin->root.display = XOpenDisplay(0);
  if(!dpawin->root.display){
    fprintf(stderr, "Failed to open X display %s\n", XDisplayName(0));
    goto error;
  }
  dpawin->root.window.xwindow = DefaultRootWindow(dpawin->root.display);
  if(!dpawin->root.window.xwindow){
    fprintf(stderr, "DefaultRootWindow failed\n");
    goto error;
  }
  wm_detected = false;
  XSetErrorHandler(&wm_check);
  XSelectInput(
      dpawin->root.display,
      dpawin->root.window.xwindow,
      SubstructureRedirectMask | SubstructureNotifyMask);
  XSync(dpawin->root.display, false);
  if(wm_detected){
    fprintf(stderr, "Detected another window manager on display %s\n", XDisplayName(0));
    goto error;
  }
  XSetErrorHandler(&dpawin_error_handler);
  if(dpawindow_register(&dpawin->root.window) == -1){
    fprintf(stderr, "Failed to register root window\n");
    return -1;
  }
  return 0;
error:
  dpawin_cleanup(dpawin);
  return -1;
}

int dpawin_error_handler(Display* display, XErrorEvent* error){
  char error_message[1024];
  if(!XGetErrorText(display, error->error_code, error_message, sizeof(error_message)))
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

int dpawin_run(struct dpawin* dpawin){
  while(true){
    XEvent event;
    XNextEvent(dpawin->root.display, &event);
    struct dpawindow* it;
    for(it = &dpawin->root.window; it; it=it->next)
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
      fprintf(stderr, "Got event %d (%s) for unknown window\n", event.type, dpawin_get_event_name(event.type));
    }
  }
  return 0;
}
