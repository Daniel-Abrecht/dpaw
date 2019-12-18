#include <atom.h>
#include <dpaw.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <signal.h>

static volatile enum dpaw_state {
  DPAW_KEEP_RUNNING,
  DPAW_STOP,
  DPAW_ERROR,
  DPAW_RESTART,
} running_state;

int dpaw_cleanup(struct dpaw* dpaw){
  printf("Stopping dpaw...\n");
  if(dpaw->root.window.xwindow)
    dpawindow_root_cleanup(&dpaw->root);
  if(dpaw->root.display)
    XCloseDisplay(dpaw->root.display);
  return 0;
}

static int takeover_existing_windows(struct dpaw* dpaw){
  XGrabServer(dpaw->root.display);
  Window root_ret, parent_ret;
  Window* window_list;
  unsigned int window_count;
  if(!XQueryTree(
    dpaw->root.display,
    dpaw->root.window.xwindow,
    &root_ret,
    &parent_ret,
    &window_list,
    &window_count
  )) return -1;
  if(root_ret != dpaw->root.window.xwindow)
    return -1;
  for(size_t i=0; i<window_count; i++){
    XWindowAttributes attribute;
    if(!XGetWindowAttributes(dpaw->root.display, window_list[i], &attribute)){
      fprintf(stderr, "XGetWindowAttributes failed\n");
      continue;
    }
    if(attribute.override_redirect || attribute.map_state != IsViewable){
      printf("Not managing window %lx, either an override_redirect window or not viewable\n", window_list[i]);
      continue;
    }
    if(dpaw_workspace_manager_manage_window(&dpaw->root.workspace_manager, window_list[i])){
      fprintf(stderr, "dpaw_workspace_manager_manage_window failed\n");
    }
  }
  if(window_list)
    XFree(window_list);
  XUngrabServer(dpaw->root.display);
  return 0;
}

void onsigterm(int x){
  (void)x;
  running_state = DPAW_STOP;
}

void onsighup(int x){
  (void)x;
  running_state = DPAW_RESTART;
}

int dpaw_init(struct dpaw* dpaw){
  signal(SIGTERM, onsigterm);
  signal(SIGINT, onsigterm);
  signal(SIGHUP, onsighup);
  XSetErrorHandler(&dpaw_error_handler);
  memset(dpaw, 0, sizeof(*dpaw));
  dpaw->root.display = XOpenDisplay(0);
  if(!dpaw->root.display){
    fprintf(stderr, "Failed to open X display %s\n", XDisplayName(0));
    goto error;
  }
  dpaw->x11_fd = ConnectionNumber(dpaw->root.display);
  if(dpaw->x11_fd == -1){
    fprintf(stderr, "ConnectionNumber failed\n");
    goto error;
  }
  dpaw->root.window.xwindow = DefaultRootWindow(dpaw->root.display);
  if(!dpaw->root.window.xwindow){
    fprintf(stderr, "DefaultRootWindow failed\n");
    goto error;
  }
  if(dpaw_atom_init(dpaw->root.display) == -1){
    fprintf(stderr, "dpaw_atom_init failed\n");
    goto error;
  }
  if(dpawindow_root_init(dpaw, &dpaw->root) == -1){
    fprintf(stderr, "dpawindow_root_init failed\n");
    goto error;
  }
  if(takeover_existing_windows(dpaw)){
    fprintf(stderr, "takeover_existing_windows failed\n");
    goto error;
  }
  return 0;
error:
  dpaw_cleanup(dpaw);
  return -1;
}

int dpaw_error_handler(Display* display, XErrorEvent* error){
  extern bool dpaw_xerror_occured;
  dpaw_xerror_occured = true;
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

int dpaw_run(struct dpaw* dpaw){
  bool debug_x_events = !!getenv("DEBUG_X_EVENTS");
  while(running_state == DPAW_KEEP_RUNNING){

    fd_set fdset;
    FD_ZERO(&fdset);
    FD_SET(dpaw->x11_fd, &fdset);

    if(!XPending(dpaw->root.display))
      select(dpaw->x11_fd+1, &fdset, 0, 0, 0);
    if(!XPending(dpaw->root.display))
      continue;

    XEvent event;
    XNextEvent(dpaw->root.display, &event);

    if(debug_x_events){
      printf(
        "XEvent: %d serial: %lu window: %lu %lu\n",
        event.type,
        event.xany.serial,
        event.xany.window, // This is actually the parent window. It should be there for every event, but like always, there are exceptions, like keyboard and generic events...
        (&event.xany.window)[1] // This may not be a window, but it often is, and the XEvent is always big enough for this, so whatever...
      );
    }

    for(struct xev_event_extension* it=dpaw_event_extension_list; it; it=it->next){
      if(!it->initialised)
        continue;
      if(it->preprocess_event)
        it->preprocess_event(dpaw, &event);
    }

    struct xev_event xev;
    if(dpaw_xevent_to_xev(dpaw, &xev, &event.xany) == -1){
      fprintf(stderr, "dpaw_xevent_to_xev failed\n");
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

    enum event_handler_result result = EHR_UNHANDLED;
    if(xev.info->event_list->extension->dispatch && (result == EHR_UNHANDLED || result == EHR_NEXT)){
      result = xev.info->event_list->extension->dispatch(dpaw, &xev);
    }
    if(result == EHR_UNHANDLED || result == EHR_NEXT){
      result = dpawindow_dispatch_event(&dpaw->root.window, &xev);
    }

    switch(result){
      case EHR_FATAL_ERROR: {
        fprintf(
          stderr,
          "A fatal error occured while trying to handle event %d %s.\n",
          event.type,
          xev.info->name
        );
        running_state = DPAW_ERROR;
      } break;
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

    dpaw_free_xev(&xev);

    while(dpaw->window_update_list.first){
      struct dpawindow* window = container_of(dpaw->window_update_list.first, struct dpawindow, dpaw_window_update_entry);
      dpaw_linked_list_set(0, dpaw->window_update_list.first, 0);
      dpawindow_deferred_update(window);
    }

  }
  if(running_state == DPAW_ERROR)
    return -1;
  if(running_state == DPAW_RESTART)
    return 1;
  return 0;
}

