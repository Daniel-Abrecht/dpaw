#include <-dpaw/dpaw.h>
#include <-dpaw/atom.h>
#include <-dpaw/font.h>
#include <-dpaw/process.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <poll.h>
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

volatile bool got_sigchld = false;


int dpaw_cleanup(struct dpaw* dpaw){
  if(!dpaw->initialised)
    return 0;
  dpaw->initialised = false;
  printf("Stopping dpaw...\n");
  if(dpaw->root.window.xwindow)
    dpawindow_cleanup(&dpaw->root.window);
  if(dpaw->root.display){
    XSync(dpaw->root.display, false);
    XCloseDisplay(dpaw->root.display);
  }
  dpaw_array_free(&dpaw->input_list);
  dpaw_array_free(&dpaw->fd_list);
  memset(dpaw, 0, sizeof(*dpaw));
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
    if(dpaw_workspace_manager_manage_window(&dpaw->root.workspace_manager, window_list[i], 0)){
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

void onsigchld(int x){
  (void)x;
  got_sigchld = true;
}

int dpaw_init(struct dpaw* dpaw){
  memset(dpaw, 0, sizeof(*dpaw));
  dpaw->initialised = true;
  if(dpaw_array_init(&dpaw->input_list, 16, true)){
    fprintf(stderr, "dpaw_array_prealloc failed");
    return -1;
  }
  if(dpaw_array_init(&dpaw->fd_list, 16, true)){
    fprintf(stderr, "dpaw_array_prealloc failed");
    return -1;
  }
  signal(SIGTERM, onsigterm);
  signal(SIGINT, onsigterm);
  signal(SIGHUP, onsighup);
  signal(SIGCHLD, onsigchld);
  XSetErrorHandler(&dpaw_error_handler);
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
  if(dpaw_poll_add(dpaw, (&(struct dpaw_fd){ .fd = dpaw->x11_fd, .keep=true }), POLLIN)){
    fprintf(stderr, "dpaw_poll_add for x11 fd failed\n");
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
  if(dpaw_font_init(dpaw->root.display) == -1){
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

int dpaw_poll_add(struct dpaw* dpaw, const struct dpaw_fd* input, int events){
  if(dpaw_array_add(&dpaw->input_list, input))
    goto error;
  if(dpaw_array_add(&dpaw->fd_list, (&(struct pollfd){
    .fd = input->fd,
    .events = events
  }))){
    dpaw_array_remove(&dpaw->fd_list, dpaw->input_list.count-1, 1);
    goto error;
  }
  assert(dpaw->input_list.count == dpaw->fd_list.count);
  return 0;
error:
  assert(dpaw->input_list.count == dpaw->fd_list.count);
  return -1;
}

void dpaw_poll_remove(struct dpaw* dpaw, const struct dpaw_fd* input){
  for(size_t i=dpaw->input_list.count; i--;){
    if(input->fd >= 0 && input->fd != dpaw->input_list.data[i].fd)
      continue;
    if(input->ptr && input->ptr != dpaw->input_list.data[i].ptr)
      continue;
    if(input->callback && input->callback != dpaw->input_list.data[i].callback)
      continue;
    dpaw_array_remove(&dpaw->input_list, i, 1);
    dpaw_array_remove(&dpaw->fd_list, i, 1);
    break;
  }
  assert(dpaw->input_list.count == dpaw->fd_list.count);
}

int dpaw_run(struct dpaw* dpaw){
  bool debug_x_events = !!getenv("DEBUG_X_EVENTS");

  while(running_state == DPAW_KEEP_RUNNING){

    if(got_sigchld){
      int status = 0;
      pid_t child = 0;
      while((child=waitpid(-1, &status, WNOHANG)) > 0){
        printf("Child %ld died\n", (long)child);
        for(struct dpaw_list_entry *it = dpaw->process_list.first; it; it=it->next){
          struct dpaw_process* process = container_of(it, struct dpaw_process, dpaw_process_entry);
          if(process->pid != child)
            continue;
          process->pid = -1;
          DPAW_CALL_BACK_AND_REMOVE(dpaw_process, process, exited, &status);
          // process and it may have been freed now
          break;
        }
      }
    }

    if(!XPending(dpaw->root.display)){
      dpaw_array_gc(&dpaw->input_list);
      dpaw_array_gc(&dpaw->fd_list);
      assert(dpaw->fd_list.count == dpaw->input_list.count);
//      printf("polling %zu entries\n", dpaw->fd_list.count);
      int ret = poll(dpaw->fd_list.data, dpaw->fd_list.count, -1);
//      printf("poll return with %d\n", ret);
      if( ret == -1 && errno != EINTR ){
        perror("poll failed");
        running_state = DPAW_ERROR;
        break;
      }
      if(ret > 0)
      for(size_t i=dpaw->fd_list.count; i--; ){
        int fd = dpaw->fd_list.data[i].fd;
        struct pollfd* pfd = dpaw->fd_list.data + i;
        struct dpaw_fd* dfd = dpaw->input_list.data + i;
        if(pfd->revents & pfd->events){
          if(dfd->callback)
            dfd->callback(dpaw, fd, pfd->revents, dfd->ptr);
          if(!dfd->keep && dpaw->fd_list.count && fd == dpaw->fd_list.data[i].fd){
            dpaw_array_remove(&dpaw->fd_list, i, 1);
            dpaw_array_remove(&dpaw->input_list, i, 1);
            continue;
          }
        }
        if(pfd->revents & (POLLERR|POLLHUP|POLLNVAL)){
          if(pfd->revents & (POLLERR|POLLNVAL))
            fprintf(stderr, "Warning: got POLLERR or POLLNVAL for fd %d\n", fd);
          if(dfd->callback){
            dfd->callback(dpaw, fd, pfd->revents, 0);
          }else if(pfd->revents & POLLHUP){
            close(fd);
          }
          if(dpaw->fd_list.count && fd == dpaw->fd_list.data[i].fd){
            dpaw_array_remove(&dpaw->fd_list, i, 1);
            dpaw_array_remove(&dpaw->input_list, i, 1);
            printf("Removing entry %zu fd %d\n", i, fd);
          }
          continue;
        }
        pfd->revents = 0;
      }
      assert(dpaw->fd_list.count == dpaw->input_list.count);
    }
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

