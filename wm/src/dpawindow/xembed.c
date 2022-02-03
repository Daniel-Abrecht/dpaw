#include <-dpaw/dpaw.h>
#include <-dpaw/dpawindow/xembed.h>
#include <-dpaw/atom.h>
#include <-dpaw/xev/X.c>
#include <-dpaw/atom/xembed.c>
#include <-dpaw/atom/icccm.c>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <poll.h>
#include <stdio.h>

DEFINE_DPAW_DERIVED_WINDOW(xembed)

static void send_xembed_message(
  struct dpawindow_xembed* xembed,
  long message,
  long detail,
  long data1,
  long data2
){
  XEvent ev;
  memset(&ev, 0, sizeof(ev));
  ev.xclient.type = ClientMessage;
  ev.xclient.window = xembed->window.xwindow;
  ev.xclient.message_type = _XEMBED;
  ev.xclient.format = 32;
  ev.xclient.data.l[0] = CurrentTime;
  ev.xclient.data.l[1] = message;
  ev.xclient.data.l[2] = detail;
  ev.xclient.data.l[3] = data1;
  ev.xclient.data.l[4] = data2;
  XSendEvent(xembed->window.dpaw->root.display, xembed->window.xwindow, False, NoEventMask, &ev);
}

static void app_cleanup_handler(
  struct dpawindow* app_window,
  void* a, void* b
){
  (void)a;
  (void)b;
  struct dpawindow_app* app = container_of(app_window, struct dpawindow_app, window);
  struct dpawindow_xembed* xembed = container_of(app, struct dpawindow_xembed, parent);
  if(app_window->xwindow){
    XDestroyWindow(app_window->dpaw->root.display, app_window->xwindow);
    app_window->xwindow = 0;
  }
  if(!xembed->window.cleanup)
    dpawindow_cleanup(&xembed->window);
}

static void parent_boundary_changed(
  struct dpawindow* app_window,
  void* a, void* b
){
  (void)a;
  (void)b;
  struct dpawindow_app* app = container_of(app_window, struct dpawindow_app, window);
  struct dpawindow_xembed* xembed = container_of(app, struct dpawindow_xembed, parent);
  if(!xembed->window.xwindow)
    return;
  struct dpaw_rect boundary = {
    .top_left.x = 0,
    .top_left.y = 0,
    .bottom_right.x = app_window->boundary.bottom_right.x - app_window->boundary.top_left.x,
    .bottom_right.y = app_window->boundary.bottom_right.y - app_window->boundary.top_left.y,
  };
  dpawindow_place_window(&xembed->window, boundary);
}

static int got_new_window(struct dpawindow_app* app, Window xwindow){
  struct dpawindow_xembed* xembed = container_of(app, struct dpawindow_xembed, parent);
  dpaw_process_cleanup(&xembed->process);
  dpawindow_xembed_set(xembed, xwindow);
  return 0;
}

static void abort_starting_applications(struct dpawindow_xembed* xembed){
  DPAW_CALLBACK_REMOVE(&xembed->new_window);
  dpaw_process_kill(&xembed->process, SIGTERM);
  dpaw_process_cleanup(&xembed->process);
  if(xembed->stdout){
    dpaw_poll_remove(xembed->parent.window.dpaw, (&(struct dpaw_fd){ .fd = xembed->stdout-1 }));
    close(xembed->stdout-1);
  }
  xembed->stdout = 0;
}

int dpawindow_xembed_init(
  struct dpaw* dpaw,
  struct dpawindow_xembed* xembed
){
  xembed->window.type = &dpawindow_type_xembed;
  xembed->window.dpaw = dpaw;
  xembed->parent.window.dpaw = dpaw;
  Window xwindow = XCreateWindow(
    dpaw->root.display, dpaw->root.window.xwindow,
    0, 0, 1, 1, 0,
    CopyFromParent, InputOutput,
    CopyFromParent, CWBackPixel, &(XSetWindowAttributes){0}
  );
  if(!xwindow){
    fprintf(stderr, "XCreateWindow failed\n");
    return -1;
  }
#define AL(X) (void*)X, sizeof(X)/sizeof(*X)
  XChangeProperty(dpaw->root.display, xwindow, WM_PROTOCOLS, XA_ATOM, 32, PropModeReplace, AL(((Atom[]){
    WM_DELETE_WINDOW
  })));
#undef AL
  if(dpawindow_app_init(dpaw, &xembed->parent, xwindow))
    return -1;
  xembed->pre_cleanup.callback = app_cleanup_handler;
  DPAW_CALLBACK_ADD(dpawindow, &xembed->parent.window, pre_cleanup, &xembed->pre_cleanup);
  xembed->parent_boundary_changed.callback = parent_boundary_changed;
  DPAW_CALLBACK_ADD(dpawindow, &xembed->parent.window, boundary_changed, &xembed->parent_boundary_changed);
  xembed->parent.got_foreign_window = got_new_window;
  return 0;
}

static void xembed_exec_take_first_window_of_process(struct dpawindow_root* root, void* pxembed, void* pvwindow){
  Window* pwindow = pvwindow;
  if(!*pwindow)
    return;
  struct dpawindow_xembed* xembed = pxembed;
  pid_t pid = dpaw_try_get_xwindow_pid(root->display, *pwindow);
  if(!pid) return;
  // TODO: check parent windows too
  if(xembed->process.pid == pid){
    dpaw_process_cleanup(&xembed->process);
    dpawindow_xembed_set(xembed, *pwindow);
    *pwindow = 0;
  }
}

static void xembed_exec_got_input(struct dpaw* dpaw, int fd, int events, void* pxembed){
  (void)dpaw;
  if(!(events & POLLIN))
    return;
  struct dpawindow_xembed* xembed = pxembed;
  assert(xembed->stdout-1 == fd);
  char buf[65] = {0};
  long long xwindow = 0;
  while(true){
    ssize_t size = read(fd, buf, sizeof(buf)-1);
    if(size == -1 && errno == EINTR)
      continue;
    if(size <= 0 || size >= (ssize_t)sizeof(buf))
      goto error;
    break;
  }
  xwindow = strtoll(buf, 0, 0);
  if(!xwindow)
    goto error;
  dpaw_process_cleanup(&xembed->process);
  close(fd);
  xembed->stdout = 0;
  dpawindow_xembed_set(xembed, xwindow);
  return;
error:
  abort_starting_applications(xembed);
}

int dpawindow_xembed_exec_v(
  struct dpawindow_xembed* xembed,
  enum dpaw_xembed_exec_id_exchange_method exmet,
  struct dpaw_process_create_options options
){

  abort_starting_applications(xembed);

  switch(exmet){

    case XEMBED_UNSUPPORTED_TAKE_FIRST_WINDOW: {
      if(dpaw_process_create_v(xembed->parent.window.dpaw, &xembed->process, &options)){
        fprintf(stderr, "dpaw_process_create failed\n");
        return -1;
      }
      xembed->new_window.callback = xembed_exec_take_first_window_of_process;
      xembed->new_window.regptr = xembed;
      DPAW_CALLBACK_ADD(dpawindow_root, &xembed->parent.window.dpaw->root, window_mapped, &xembed->new_window);
    } return 0;

    case XEMBED_METHOD_GIVE_WINDOW_BY_ARGUMENT: {
      size_t count = 0;
      for(const char*const* it=options.args; *it; it++)
        count++;
      bool had_xid_arg = false;
      char xid_str[sizeof(Window)*2+3] = {0};
      snprintf(xid_str, sizeof(xid_str), "0x%llx", (long long)xembed->parent.window.xwindow);
      const char** args = malloc(sizeof(char*[count+1]));
      if(!args)
        return -1;
      for(const char *const* src=options.args, ** dst=args; *src; src++,dst++){
        if(!strcmp(*src, "<XID>")){
          had_xid_arg = true;
          *dst = xid_str;
        }else{
          *dst = *src;
        }
      }
      args[count] = 0;
      if(had_xid_arg){
        options.args = args;
        if(dpaw_process_create_v(xembed->parent.window.dpaw, &xembed->process, &options)){
          fprintf(stderr, "dpaw_process_create failed\n");
          free(args);
          return -1;
        }
        free(args);
      }else{
        free(args);
        return -1;
      }
    }; return 0;

    case XEMBED_METHOD_TAKE_WINDOW_FROM_STDOUT: {
      int process_stdout[2];
      if(pipe(process_stdout)){
        perror("pipe failed");
        return -1;
      }
      if(fcntl(process_stdout[0], F_SETFD, FD_CLOEXEC) == -1){
        perror("fcntl(process_stdout[0], F_SETFD, FD_CLOEXEC) failed");
        close(process_stdout[0]);
        close(process_stdout[1]);
        return -1;
      }
      dpaw_int_pair_t* fdmap = calloc(options.fdmap_count+1, sizeof(dpaw_int_pair_t));
      memcpy(fdmap, options.fdmap, options.fdmap_count * sizeof(dpaw_int_pair_t));
      fdmap[options.fdmap_count][0] = process_stdout[1];
      fdmap[options.fdmap_count][1] = STDOUT_FILENO;
      options.fdmap_count += 1;
      options.fdmap = fdmap;
      if(dpaw_process_create_v(xembed->parent.window.dpaw, &xembed->process, &options)){
        fprintf(stderr, "dpaw_process_create failed\n");
        close(process_stdout[0]);
        close(process_stdout[1]);
        free(fdmap);
        return -1;
      }
      free(fdmap);
      close(process_stdout[1]);
      if(dpaw_poll_add(xembed->parent.window.dpaw, (&(struct dpaw_fd){
        .fd = process_stdout[0],
        .callback = xembed_exec_got_input,
        .ptr = xembed
      }), POLLIN)){
        fprintf(stderr, "dpaw_poll_add failed\n");
        close(process_stdout[0]);
        return -1;
      };
      xembed->stdout = process_stdout[0] + 1;
    } return 0;

  }
  return -1;
}

int xembed_update_info(dpawindow_xembed* xembed){
  long* res = 0;
  xembed->info.version = 0;
  xembed->info.flags = XEMBED_MAPPED;
  if(dpaw_get_property(&xembed->window, _XEMBED_INFO, (size_t[]){8}, 0, (void**)&res) == -1){
    fprintf(stderr, "dpaw_get_property _XEMBED_INFO failed\n");
    if(res) XFree(res);
    return -1;
  }
  if(res){
    xembed->info.version = res[0];
    xembed->info.flags = res[1];
    XFree(res);
  }
  if(!xembed->ready && (xembed->info.flags & XEMBED_MAPPED)){
    xembed->ready = false;
    if(!dpawindow_get_reasonable_size_hints(&xembed->window, &xembed->parent.observable.desired_placement.value)){
      DPAW_APP_OBSERVABLE_NOTIFY(&xembed->parent, desired_placement);
    }
    parent_boundary_changed(&xembed->parent.window, 0, 0);
    dpawindow_set_mapping(&xembed->window, true);
  }
  return 0;
}

EV_ON(xembed, PropertyNotify){
  if(!event->atom)
    return EHR_ERROR;
  {
    char* name = XGetAtomName(window->window.dpaw->root.display, event->atom);
    printf("PropertyNotify %s %d\n", name, event->state);
    if(name) XFree(name);
  }
  if(event->atom == _XEMBED_INFO){
    xembed_update_info(window);
  }else{
    if(dpawindow_app_update_properties(&window->parent, &window->window, event->atom))
      return EHR_OK;
  }
  return EHR_UNHANDLED;
}

int dpawindow_xembed_set(struct dpawindow_xembed* xembed, Window xwindow){
  if(xembed->window.xwindow == xwindow)
    return 0;

  if(xembed->window.xwindow){
    dpawindow_set_mapping(&xembed->window, false);
    dpawindow_hide(&xembed->window, true);
    dpawindow_unregister(&xembed->window);
    XKillClient(xembed->window.dpaw->root.display, xembed->window.xwindow);
    xembed->window.xwindow = 0;
    xembed->ready = false;
  }

  abort_starting_applications(xembed);

  if(xwindow){
    xembed->window.xwindow = xwindow;
    XReparentWindow(xembed->parent.window.dpaw->root.display, xembed->window.xwindow, xembed->parent.window.xwindow, 0, 0);
    if(dpawindow_register(&xembed->window) != 0){
      fprintf(stderr, "dpawindow_register failed\n");
      return -1;
    }
    dpawindow_app_update_properties(&xembed->parent, &xembed->window, 0);
    xembed_update_info(xembed);
    long min_version = xembed->info.version < DPAW_XEMBED_VERSION ? xembed->info.version : DPAW_XEMBED_VERSION;
    send_xembed_message(xembed, XEMBED_EMBEDDED_NOTIFY, 0, xembed->window.xwindow, min_version);
    dpawindow_hide(&xembed->window, false);
  }

  DPAW_CALL_BACK(dpawindow_xembed, xembed, window_changed, 0);

  return 0;
}

static void dpawindow_xembed_cleanup(struct dpawindow_xembed* xembed){
  if(!xembed->parent.window.cleanup){
    dpawindow_cleanup(&xembed->parent.window);
  }else{
    dpaw_linked_list_move(&xembed->parent.window.post_cleanup.list, &xembed->window.post_cleanup.list, 0);
  }
  dpawindow_xembed_set(xembed, 0);
  dpaw_linked_list_clear(&xembed->window_changed.list);
}
