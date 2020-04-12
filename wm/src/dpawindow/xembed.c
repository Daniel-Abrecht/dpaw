#include <dpaw/dpaw.h>
#include <dpaw/dpawindow/xembed.h>
#include <dpaw/xev/X.c>

#include <signal.h>
#include <stdio.h>

DEFINE_DPAW_DERIVED_WINDOW(xembed)

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

int dpawindow_xembed_exec_v(
  struct dpawindow_xembed* xembed,
  enum dpaw_xembed_exec_id_exchange_method exmet,
  struct dpaw_process_create_options options
){
  switch(exmet){

    case XEMBED_UNSUPPORTED_TAKE_FIRST_WINDOW: {
      if(dpaw_process_create_v(xembed->parent.window.dpaw, &xembed->process, &options)){
        fprintf(stderr, "dpaw_process_create failed\n");
        return -1;
      }
      xembed->new_window.callback = xembed_exec_take_first_window_of_process;
      xembed->new_window.regptr = xembed;
      DPAW_CALLBACK_ADD(dpawindow_root, &xembed->parent.window.dpaw->root, window_mapped, &xembed->new_window);
    } break;

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
    }; break;

    case XEMBED_METHOD_TAKE_WINDOW_FROM_STDOUT: {
      puts("XEMBED_METHOD_TAKE_WINDOW_FROM_STDOUT isn't implemented yet");
      return -1; // TODO
    } break;

  }
  return 0;
}

int dpawindow_xembed_set(struct dpawindow_xembed* xembed, Window xwindow){
  if(xembed->window.xwindow == xwindow)
    return 0;

  if(xembed->window.xwindow){
    dpawindow_unregister(&xembed->window);
    XKillClient(xembed->window.dpaw->root.display, xembed->window.xwindow);
    xembed->window.xwindow = 0;
  }

  dpaw_process_kill(&xembed->process, SIGTERM);
  dpaw_process_cleanup(&xembed->process);
  DPAW_CALLBACK_REMOVE(&xembed->new_window);

  if(xwindow){
    xembed->window.xwindow = xwindow;
    XReparentWindow(xembed->parent.window.dpaw->root.display, xembed->window.xwindow, xembed->parent.window.xwindow, 0, 0);
    if(dpawindow_register(&xembed->window) != 0){
      fprintf(stderr, "dpawindow_register failed\n");
      return -1;
    }
    dpawindow_set_mapping(&xembed->window, true);
    dpawindow_hide(&xembed->window, false);
    parent_boundary_changed(&xembed->parent.window, 0, 0);
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
