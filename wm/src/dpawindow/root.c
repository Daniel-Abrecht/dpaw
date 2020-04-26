#include <-dpaw/dpaw.h>
#include <-dpaw/xev/X.c>
#include <-dpaw/atom/ewmh.c>
#include <-dpaw/workspace.h>
#include <-dpaw/screenchange.h>
#include <-dpaw/dpawindow/root.h>
#include <X11/Xatom.h>
#include <stdio.h>
#include <stdlib.h>

DEFINE_DPAW_DERIVED_WINDOW(root)

int dpawindow_root_init(struct dpaw* dpaw, struct dpawindow_root* window){
  for(struct xev_event_extension* extension=dpaw_event_extension_list; extension; extension=extension->next){
    if(extension->init(dpaw, extension)){
      fprintf(stderr, "Failed to initialise event extension %s\n", extension->name);
      if(extension->required)
        return -1;
      continue;
    }
    extension->initialised = true;
  }

  window->window.type = &dpawindow_type_root;
  window->window.dpaw = dpaw;
  if(dpawindow_register(&window->window)){
    fprintf(stderr, "dpawindow_register failed\n");
    return -1;
  }

#define AL(X) (void*)X, sizeof(X)/sizeof(*X)
  XChangeProperty(window->display, window->window.xwindow, _NET_SUPPORTED, XA_ATOM, 32, PropModeReplace, AL(((Atom[]){
    _NET_SUPPORTED,
    _NET_SUPPORTING_WM_CHECK,

    _NET_WM_STATE,
//    _NET_WM_STATE_DEMANDS_ATTENTION,
    _NET_WM_STATE_FOCUSED,
    _NET_WM_STATE_MAXIMIZED_HORZ,
    _NET_WM_STATE_MAXIMIZED_VERT,
    _NET_WM_STATE_FULLSCREEN,
//    _NET_WM_STATE_MODAL,
    _NET_WM_STATE_HIDDEN,
    _NET_WM_STATE_SKIP_PAGER,
    _NET_WM_STATE_SKIP_TASKBAR,

    _NET_WM_WINDOW_TYPE,
    _NET_WM_WINDOW_TYPE_DESKTOP,
    _NET_WM_WINDOW_TYPE_DIALOG,
    _NET_WM_WINDOW_TYPE_DOCK,
    _NET_WM_WINDOW_TYPE_MENU,
    _NET_WM_WINDOW_TYPE_NORMAL,
    _NET_WM_WINDOW_TYPE_SPLASH,
    _NET_WM_WINDOW_TYPE_TOOLBAR,
    _NET_WM_WINDOW_TYPE_UTILITY,

//    _NET_ACTIVE_WINDOW,

    _NET_WM_ICON,
    _NET_WM_ICON_GEOMETRY,
    _NET_WM_ICON_NAME,
    _NET_WM_MOVERESIZE,
    _NET_WM_NAME,
    _NET_WM_PID,
//    _NET_WM_PING,

    _NET_VIRTUAL_ROOTS,

//    _NET_CLOSE_WINDOW,
//    _NET_MOVERESIZE_WINDOW
  })));
#undef AL

  {
    // This is just for _NET_SUPPORTING_WM_CHECK. It won't be needed for anything else later
    Window check_window = XCreateWindow(
      window->display, window->window.xwindow,
      0, 0, 1, 1, 0,
      CopyFromParent, InputOutput,
      CopyFromParent, CWBackPixel, &(XSetWindowAttributes){0}
    );
    if(!check_window){
      fprintf(stderr, "XCreateWindow failed\n");
      return -1;
    }
    XChangeProperty(window->display, window->window.xwindow, _NET_SUPPORTING_WM_CHECK, XA_WINDOW, 32, PropModeReplace, (void*)&check_window, 1);
    XChangeProperty(window->display, check_window, _NET_SUPPORTING_WM_CHECK, XA_WINDOW, 32, PropModeReplace, (void*)&check_window, 1);
  }

  if(dpaw_screenchange_init(&window->screenchange_detector, dpaw)){
    fprintf(stderr, "dpaw_screenchange_init failed\n");
    return -1;
  }
  if(dpaw_workspace_manager_init(&window->workspace_manager, dpaw) == -1){
    fprintf(stderr, "dpaw_workspace_init failed\n");
    return -1;
  }
  return 0;
}

static void dpawindow_root_cleanup(struct dpawindow_root* window){
  dpaw_workspace_manager_destroy(&window->workspace_manager);
  dpaw_screenchange_destroy(&window->screenchange_detector);
  dpaw_linked_list_clear(&window->window_mapped.list);
}

EV_ON(root, MapRequest){
  struct dpawindow* win = dpawindow_lookup(window->window.dpaw, event->window);
  printf("MapRequest %lx %p\n", event->window, (void*)win);
  if(!win){
    XWindowAttributes attribute;
    if(!XGetWindowAttributes(window->display, event->window, &attribute)){
      fprintf(stderr, "XGetWindowAttributes failed\n");
      return EHR_ERROR;
    }
    Window root=0, parent=0, *children=0;
    unsigned int num_children=0;
    if(!XQueryTree(window->display, event->window, &root, &parent, &children, &num_children))
      return EHR_ERROR;
    if(children)
      XFree(children);
    bool is_in_root_or_workspace = parent == window->window.xwindow;
    if(!attribute.override_redirect && !is_in_root_or_workspace){
      struct dpawindow* rwin = dpawindow_lookup(window->window.dpaw, parent);
      if(rwin && rwin->type->is_workspace)
        is_in_root_or_workspace = true;
    }
    if(is_in_root_or_workspace){
      // give other components an oppurtunity to take the window
      Window xwin = event->window;
      DPAW_CALL_BACK(dpawindow_root, window, window_mapped, &xwin);
      if(!xwin)
        return EHR_OK;
    }
    if(attribute.override_redirect || !is_in_root_or_workspace){
      // This isn't our window, but the window needs to be mapped
      XMapWindow(window->display, event->window);
      return EHR_NEXT;
    }
    if(dpaw_workspace_manager_manage_window(&window->workspace_manager, event->window) != 0)
      return EHR_ERROR;
  }else{
    // Probably already managed
    return EHR_NEXT;
  }
  return EHR_OK;
}

EV_ON(root, UnmapNotify){
  printf("UnmapNotify %lx\n", event->window);
  extern struct dpawindow_type dpawindow_type_app;
  struct dpawindow* win = dpawindow_lookup(window->window.dpaw, event->window);
  if(!win)
    return EHR_NEXT;
  if(win->type == &dpawindow_type_app){
    // An application is supposed to send this request manually
    // Regular unmap requests are also generated by this window manager, but shouldn't be handled here.
    if(!event->send_event)
      return EHR_NEXT;
    dpawindow_cleanup(win);
    return EHR_OK;
  }
  return EHR_NEXT;
}

EV_ON(root, DestroyNotify){
  printf("DestroyNotify %lx\n", event->window);
  struct dpawindow* win = dpawindow_lookup(window->window.dpaw, event->window);
  if(!win)
    return EHR_NEXT;
  dpawindow_cleanup(win);
  return EHR_NEXT;
}

EV_ON(root, ConfigureRequest){
  printf("ConfigureRequest %lx\n", event->window);
  XWindowChanges changes = {
    .x = event->x,
    .y = event->y,
    .width  = event->width,
    .height = event->height,
    .border_width = event->border_width,
    .sibling      = event->above,
    .stack_mode   = event->detail
  };
  XConfigureWindow(window->display, event->window, event->value_mask, &changes);
  return EHR_OK;
}

EV_ON(root, ReparentNotify){
  (void)window;
  printf("root ReparentNotify %lx\n", event->window);
  return EHR_NEXT;
}

EV_ON(root, ClientMessage){
  char* name = XGetAtomName(window->window.dpaw->root.display, event->message_type);
  printf("ClientMessage: %s\n", name);
  XFree(name);
  return EHR_NEXT;
}
