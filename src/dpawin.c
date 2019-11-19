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

int dpawin_init(struct dpawin* dpawin){
  XSetErrorHandler(&dpawin_error_handler);
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
  return 0;
error:
  dpawin_cleanup(dpawin);
  return -1;
}

int dpawin_error_handler(Display* display, XErrorEvent* error){
  extern bool dpawin_xerror_occured;
  dpawin_xerror_occured = true;
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

    if(debug_x_events){
      printf(
        "XEvent: %d serial: %lu window: %lu %lu\n",
        event.type,
        event.xany.serial,
        event.xany.window, // This is actually the parent window. It should be there for every event, but like always, there are exceptions, like keyboard and generic events...
        (&event.xany.window)[1] // This may not be a window, but it often is, and the XEvent is always big enough for this, so whatever...
      );
    }

    struct xev_event xev;
    if(dpawin_xevent_to_xev(dpawin, &xev, &event.xany) == -1){
      fprintf(stderr, "dpawin_xevent_to_xev failed\n");
      continue;
    }

    if(debug_x_events){
      printf(
        "XEvent: %d %d %s serial: %lu window: %lu %lu\n",
        event.type,
        xev.info->type,
        xev.info->name,
        event.xany.serial,
        event.xany.window, // This is actually the parent window. It should be there for every event, but like always, there are exceptions, like keyboard and generic events...
        (&event.xany.window)[1] // This may not be a window, but it often is, and the XEvent is always big enough for this, so whatever...
      );
    }

    enum event_handler_result result = dpawindow_dispatch_event(&dpawin->root.window, &xev);
    if(xev.info->event_list->extension->dispatch && (result == EHR_UNHANDLED || result == EHR_NEXT))
      result = xev.info->event_list->extension->dispatch(dpawin, &xev);

    switch(result){
      case EHR_FATAL_ERROR: {
        fprintf(
          stderr,
          "A fatal error occured while trying to handle event %d %s.\n",
          event.type,
          xev.info->name
        );
        dpawin_free_xev(&xev);
      } return -1;
      case EHR_OK: break;
      case EHR_NEXT: break;
      case EHR_ERROR: {
        fprintf(
          stderr,
          "An error occured while trying to handle event %d %s.\n",
          event.type,
          xev.info->name
        );
      } break;
      case EHR_UNHANDLED: {
        fprintf(
          stderr,
          "Got unhandled event %d %s.\n",
          event.type,
          xev.info->name
        );
      } break;
      case EHR_INVALID: {
        fprintf(
          stderr,
          "Got invalid event %d %s.\n",
          event.type,
          xev.info->name
        );
      } break;
    }

    dpawin_free_xev(&xev);
  }
  return 0;
}

