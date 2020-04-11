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
  dpawindow_xembed_cleanup(xembed); // See dpawindow_xembed_cleanup for more info
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
    0, 0, 800, 600, 0,
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
      xembed->new_window_handler.callback = xembed_exec_take_first_window_of_process;
      xembed->new_window_handler.regptr = xembed;
      DPAW_CALLBACK_ADD(dpawindow_root, &xembed->parent.window.dpaw->root, window_mapped, &xembed->new_window_handler);
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
      for(const char *const* src=options.args, ** dst=args; *src; src++){
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
  DPAW_CALLBACK_REMOVE(&xembed->new_window_handler);
  if(xwindow){
    xembed->window.xwindow = xwindow;
    XReparentWindow(xembed->parent.window.dpaw->root.display, xembed->window.xwindow, xembed->parent.window.xwindow, 0, 0);
    if(dpawindow_register(&xembed->window) != 0){
      fprintf(stderr, "dpawindow_register failed\n");
      return -1;
    }
    XMapWindow(xembed->parent.window.dpaw->root.display, xembed->window.xwindow);
  }
  DPAW_CALL_BACK(dpawindow_xembed, xembed, window_changed, 0);
  return 0;
}

static void dpawindow_xembed_cleanup(struct dpawindow_xembed* xembed){

  // This window and it's parent share a lifetime.
  // The destruction of the parent window will call this function as last thing of the pre_cleanup.
  // dpawindow_cleanup will also call the parents pre_cleanup list, but if it was already called,
  // it'll be empty, it won't reexecute this function.
  // This function is always called before the post_cleanup of the parent.
  // The post_cleanup of the parent is added to this function's post_cleanup to make sure
  // it's run after this function, and before this functions post_cleanup handlers.
  dpaw_linked_list_move(&xembed->window.post_cleanup.list, &xembed->parent.window.post_cleanup.list, xembed->window.post_cleanup.list.first);
  dpawindow_cleanup(&xembed->parent.window);

  dpawindow_xembed_set(xembed, 0);
  dpaw_linked_list_clear(&xembed->window_changed.list);
  if(xembed->parent.window.xwindow)
    XDestroyWindow(xembed->parent.window.dpaw->root.display, xembed->parent.window.xwindow);
  memset(xembed, 0, sizeof(*xembed));

}
