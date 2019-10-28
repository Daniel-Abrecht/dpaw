#include <dpawin.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

int dpawin_cleanup(struct dpawin* dpawin){
  if(dpawin->root.display)
    XCloseDisplay(dpawin->root.display);
  if(dpawin->root.window.xwindow)
    dpawindow_root_cleanup(&dpawin->root);
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
  if(dpawindow_root_init(dpawin, &dpawin->root) == -1){
    fprintf(stderr, "dpawindow_root_init failed\n");
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
//  XSynchronize(dpawin->root.display, True);
  XSetErrorHandler(&dpawin_error_handler);
  return 0;
error:
  dpawin_cleanup(dpawin);
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

int dpawin_run(struct dpawin* dpawin){
  bool debug_x_events = !!getenv("DEBUG_X_EVENTS");
  while(true){
    XEvent event;
    XNextEvent(dpawin->root.display, &event);
    int extension = -1;
    int event_type = event.type;
    if(event_type == GenericEvent){
      if(event.xgeneric.extension == -1){
        fprintf(stderr, "Warning: Got generic event with extension -1. Can't handle this, I have choosen -1 as a sentinel for no extension, assuming that to be an invalid extension number\n");
        continue;
      }else{
        extension = event.xgeneric.extension;
        event_type = event.xgeneric.evtype;
      }
    }
    if(debug_x_events){
      printf(
        "XEvent: %d %s serial: %lu window: %lu %lu\n",
        event.type,
        dpawin_get_event_name(dpawin, extension, event.type),
        event.xany.serial,
        event.xany.window, // This is actually the parent window. It should be there for every event, but like always, there are exceptions, like keyboard and generic events...
        (&event.xany.window)[1] // This may not be a window, but it often is, and the XEvent is always big enough for this, so whatever...
      );
    }

    enum event_handler_result result = EHR_FATAL_ERROR;
    struct dpawin_xev* xev = dpawin_get_event_extension(dpawin, extension);
    if(!xev || !xev->xev){
      result = EHR_INVALID;
    }else{
      // Let's first try to dispatch it on the root window
      void* data = &event;
      if(XGetEventData(dpawin->root.display, &event.xcookie))
        data = event.xcookie.data;
      result = dpawindow_dispatch_event(&dpawin->root.window, xev->xev, event_type, data);
      if(xev->xev->dispatch && (result == EHR_UNHANDLED || result == EHR_NEXT))
        result = xev->xev->dispatch(dpawin, xev, event_type, data);
      XFreeEventData(dpawin->root.display, &event.xcookie);
    }

    switch(result){
      case EHR_FATAL_ERROR: {
        fprintf(
          stderr,
          "A fatal error occured while trying to handle event %d::%d %s::%s.\n",
          extension,
          event_type,
          dpawin_get_extension_name(dpawin, extension),
          dpawin_get_event_name(dpawin, extension, event_type)
        );
      } return -1;
      case EHR_OK: break;
      case EHR_NEXT: break;
      case EHR_ERROR: {
        fprintf(
          stderr,
          "An error occured while trying to handle event %d::%d %s::%s.\n",
          extension,
          event_type,
          dpawin_get_extension_name(dpawin, extension),
          dpawin_get_event_name(dpawin, extension, event_type)
        );
      } break;
      case EHR_UNHANDLED: {
        fprintf(
          stderr,
          "Got unhandled event %d::%d %s::%s.\n",
          extension,
          event_type,
          dpawin_get_extension_name(dpawin, extension),
          dpawin_get_event_name(dpawin, extension, event_type)
        );
      } break;
      case EHR_INVALID: {
        fprintf(
          stderr,
          "Got invalid event %d::%d %s::%s.\n",
          extension,
          event_type,
          dpawin_get_extension_name(dpawin, extension),
          dpawin_get_event_name(dpawin, extension, event_type)
        );
      } break;
    }

  }
  return 0;
}
