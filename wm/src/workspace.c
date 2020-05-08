#include <-dpaw/dpaw.h>
#include <-dpaw/xev/X.c>
#include <-dpaw/atom/ewmh.c>
#include <-dpaw/workspace.h>
#include <-dpaw/screenchange.h>
#include <-dpaw/dpawindow/app.h>
#include <-dpaw/dpawindow/root.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

// TODO: Replace with dpaw_list
static struct dpaw_workspace_type* workspace_type_list;

int dpaw_workspace_set_focus(struct dpawindow_app* window){
  if(!window->workspace)
    return -1;
  XSetInputFocus(window->window.dpaw->root.display, window->window.xwindow, RevertToPointerRoot, CurrentTime);
  window->workspace->focus_window = window;
  return 0;
}

EV_ON(root, MapNotify){
  extern struct dpawindow_type dpawindow_type_app;
  struct dpawindow* win = dpawindow_lookup(window->window.dpaw, event->window);
  if(!win)
    return EHR_NEXT;
  if(win->type != &dpawindow_type_app)
    return EHR_NEXT;
  struct dpawindow_app* app = container_of(win, struct dpawindow_app, window);
  if(app->workspace && app->workspace->focus_window == app)
    dpaw_workspace_set_focus(app);
  return EHR_OK;
}

static void screenchange_handler(void* ptr, enum dpaw_screenchange_type what, const struct dpaw_screen_info* info){
  struct dpaw_workspace_manager* wmgr = ptr;
  printf(
    "Screen %p %d (%ld-%ld)x(%ld-%ld)\n",
    (void*)info,
    what,
    info->boundary.bottom_right.x,
    info->boundary.top_left.x,
    info->boundary.bottom_right.y,
    info->boundary.top_left.y
  );
  switch(what){
    case DPAW_SCREENCHANGE_SCREEN_ADDED: {
      struct dpaw_workspace_screen* screen = calloc(sizeof(struct dpaw_workspace_screen), 1);
      if(!screen){
        fprintf(stderr, "Failed to allocate space for dpaw_workspace_screen object\n");
        return;
      }
      dpaw_workspace_screen_init(wmgr, screen, info);
      dpaw_workspace_manager_designate_screen_to_workspace(wmgr, screen);
    } break;
    case DPAW_SCREENCHANGE_SCREEN_CHANGED: {
      struct dpaw_list_entry* sit = 0;
      for(struct dpaw_list_entry* wlit = wmgr->workspace_list.first; wlit; wlit=wlit->next)
        for(sit = container_of(wlit, struct dpaw_workspace, wmgr_workspace_list_entry)->screen_list.first; sit; sit=sit->next )
          if( container_of(sit, struct dpaw_workspace_screen, workspace_screen_entry)->info == info )
            break;
      if(!sit){
        fprintf(stderr, "Warning: dpaw_workspace_screen object to be updated not found\n");
        return;
      }
      struct dpaw_workspace_screen* wscr = container_of(sit, struct dpaw_workspace_screen, workspace_screen_entry);
      dpaw_workspace_manager_designate_screen_to_workspace(wmgr, wscr);
    } break;
    case DPAW_SCREENCHANGE_SCREEN_REMOVED: {
      struct dpaw_list_entry* sit = 0;
      for(struct dpaw_list_entry* wlit = wmgr->workspace_list.first; wlit; wlit=wlit->next)
        for(sit = container_of(wlit, struct dpaw_workspace, wmgr_workspace_list_entry)->screen_list.first; sit; sit=sit->next )
          if( container_of(sit, struct dpaw_workspace_screen, workspace_screen_entry)->info == info )
            break;
      if(!sit){
        fprintf(stderr, "Warning: dpaw_workspace_screen object to be removed not found\n");
        return;
      }
      struct dpaw_workspace_screen* wscr = container_of(sit, struct dpaw_workspace_screen, workspace_screen_entry);
      dpaw_workspace_screen_cleanup(wscr);
      dpaw_linked_list_set(0, sit, 0);
      free(wscr);
    } break;
  }
}

int dpaw_workspace_screen_init(struct dpaw_workspace_manager* wmgr, struct dpaw_workspace_screen* screen, const struct dpaw_screen_info* info){
  (void)wmgr;
  screen->info = info;
  return 0;
}

static int update_workspace(struct dpaw_workspace* workspace){
  // Calculate the boundaries just large enough to include all screens
  if(workspace->screen_list.first){
    struct dpaw_rect boundary = container_of(workspace->screen_list.first, struct dpaw_workspace_screen, workspace_screen_entry)->info->boundary;
    for( struct dpaw_list_entry* it = workspace->screen_list.first; it; it=it->next ){
      struct dpaw_workspace_screen* wscr = container_of(it, struct dpaw_workspace_screen, workspace_screen_entry);
      if( boundary.top_left.x > wscr->info->boundary.top_left.x )
        boundary.top_left.x = wscr->info->boundary.top_left.x;
      if( boundary.top_left.y > wscr->info->boundary.top_left.y )
        boundary.top_left.y = wscr->info->boundary.top_left.y;
      if( boundary.bottom_right.x < wscr->info->boundary.bottom_right.x )
        boundary.bottom_right.x = wscr->info->boundary.bottom_right.x;
      if( boundary.bottom_right.y < wscr->info->boundary.bottom_right.y )
        boundary.bottom_right.y = wscr->info->boundary.bottom_right.y;
    }
    if(dpawindow_place_window(workspace->window, boundary))
      return -1;
  }
  return 0;
}

int dpaw_reassign_screen_to_workspace(struct dpaw_workspace_screen* screen, struct dpaw_workspace* workspace){
  if(screen->workspace == workspace){
    if(!workspace)
      return 0;
    update_workspace(workspace);
    if(workspace->type->screen_changed)
      return workspace->type->screen_changed(workspace->window, screen);
    return 0;
  }
  if(screen->workspace){
    struct dpaw_workspace* old_workspace = screen->workspace;
    dpaw_linked_list_set(0, &screen->workspace_screen_entry, 0);
    screen->workspace = 0;
    update_workspace(old_workspace);
    if(old_workspace->type->screen_removed)
      old_workspace->type->screen_removed(old_workspace->window, screen);
  }
  if(workspace){
    screen->workspace = workspace;
    dpaw_linked_list_set(&workspace->screen_list, &screen->workspace_screen_entry, 0);
    update_workspace(workspace);
    if(workspace->type->screen_added)
      if(workspace->type->screen_added(workspace->window, screen))
        return -1;
  }
  return 0;
}

struct dpaw_workspace_type* choose_best_target_workspace_type(struct dpaw_workspace_manager* wmgr, struct dpaw_workspace_screen* screen){
   (void)wmgr;
   (void)screen;
  // TODO: Come up with properties to determine the most fitting workspace type.
  // For now, let's just take a random one and worry about this later.
  return workspace_type_list;
}

static int update_virtual_root_property(struct dpaw_workspace_manager* wmgr){
  if(!wmgr->workspace_list.size)
    return 0;
  if(wmgr->workspace_list.size > 256)
    return -1; // Since root_window_list is currently allocated on the stack, let's make sure it won't become so big that it stackoverflows
  {
    Window root_window_list[wmgr->workspace_list.size]; // Don't use window, an entry it may not be 32 bit long.
    size_t i = 0;
    for(struct dpaw_list_entry* wlit = wmgr->workspace_list.first; wlit; wlit=wlit->next)
      root_window_list[i++] = container_of(wlit, struct dpaw_workspace, wmgr_workspace_list_entry)->window->xwindow;
    XChangeProperty(wmgr->dpaw->root.display, wmgr->dpaw->root.window.xwindow, _NET_VIRTUAL_ROOTS, XA_WINDOW, 32, PropModeReplace, (void*)root_window_list, wmgr->workspace_list.size);
  }
  return 0;
}

static void workspace_pre_cleanup(struct dpawindow* window, void* pworkspace, void* ptr){
  (void)window;
  (void)ptr;

  struct dpaw_workspace* workspace = pworkspace;
  struct dpaw_workspace_manager* wmgr = workspace->workspace_manager;

  dpaw_linked_list_set(0, &workspace->wmgr_workspace_list_entry, 0);
  update_virtual_root_property(wmgr);

  // xwindows won't be destroyed. dpaw_workspace_remove_window will reparent them back to the root.
  // after that, they may be taken up by the manager again, and assigned to another workspace
  while(workspace->window_list.first)
    dpawindow_cleanup(&container_of(workspace->window_list.first, struct dpawindow_app, workspace_window_entry)->window);
}

static void workspace_post_cleanup(struct dpawindow* window, void* pworkspace, void* ptr){
  (void)pworkspace;
  (void)ptr;

  struct dpaw_workspace* workspace = pworkspace;
  assert(!workspace->window_list.first);
  XDestroyWindow(window->dpaw->root.display, window->xwindow);
  free(window);
}

struct dpaw_workspace* dpawindow_to_dpaw_workspace(struct dpawindow* window){
  if(!window || !window->type->workspace_type)
    return 0;
  return (struct dpaw_workspace*)((char*)window+window->type->workspace_type->derived_offset);
}

static struct dpaw_workspace* create_workspace(struct dpaw_workspace_manager* wmgr, struct dpaw_workspace_type* type){
  if(type->size+type->derived_offset < sizeof(struct dpaw_workspace)){
    fprintf(stderr, "Error: Invalid workspace type size\n");
    return 0;
  }
  void* memory = calloc(type->size, 1);
  if(!memory){
    fprintf(stderr, "calloc failed\n");
    goto error;
  }
  struct dpaw_workspace* workspace = (struct dpaw_workspace*)((char*)memory+type->derived_offset);
  workspace->type = type;
  workspace->workspace_manager = wmgr;
  workspace->window = memory;
  workspace->pre_cleanup.callback = workspace_pre_cleanup;
  workspace->pre_cleanup.regptr = workspace;
  workspace->post_cleanup.callback = workspace_post_cleanup;
  workspace->post_cleanup.regptr = workspace;

  int screen = DefaultScreen(wmgr->dpaw->root.display);
  unsigned long black = XBlackPixel(wmgr->dpaw->root.display, screen);
  XSetWindowAttributes attrs = {
    .background_pixel = black,
  };
  Window window = XCreateWindow(
    wmgr->dpaw->root.display, wmgr->dpaw->root.window.xwindow,
    0, 0, 1, 1, 0,
    CopyFromParent, InputOutput,
    CopyFromParent, CWBackPixel, &attrs
  );
  if(!window){
    fprintf(stderr, "XCreateWindow failed\n");
    goto error;
  }
  workspace->window->xwindow = window;

  if(workspace->type->init){
    if(workspace->type->init(workspace->window)){
      fprintf(stderr, "%s::init failed\n", workspace->type->name);
      goto error_after_XCreateWindow;
    }
  }

  DPAW_CALLBACK_ADD(dpawindow, workspace->window, pre_cleanup , &workspace->pre_cleanup);
  DPAW_CALLBACK_ADD(dpawindow, workspace->window, post_cleanup, &workspace->post_cleanup);

  dpawindow_set_mapping(workspace->window, true);
  dpaw_linked_list_set(&wmgr->workspace_list, &workspace->wmgr_workspace_list_entry, 0);
  update_virtual_root_property(wmgr);

  return workspace;

error_after_XCreateWindow:
  XDestroyWindow(wmgr->dpaw->root.display, window);
error:
  if(memory)
    free(memory);
  return 0;
}

int dpaw_workspace_manager_designate_screen_to_workspace(struct dpaw_workspace_manager* wmgr, struct dpaw_workspace_screen* screen){
  if(!screen->workspace){
    struct dpaw_workspace_type* type = choose_best_target_workspace_type(wmgr, screen);
    if(!type){
      fprintf(stderr, "Error: choose_best_target_workspace_type failed\n");
      return -1;
    }
    // Does any existing workspace of the desired type want this screen?
    struct dpaw_workspace* target_workspace = 0;
    int min = 0;
    for(struct dpaw_list_entry* wlit = wmgr->workspace_list.first; wlit; wlit=wlit->next){
      struct dpaw_workspace* workspace = container_of(wlit, struct dpaw_workspace, wmgr_workspace_list_entry);
      if(workspace->type != type)
        continue;
      if(!workspace->type->screen_make_bid)
        continue;
      int res = workspace->type->screen_make_bid(workspace->window, screen);
      if(!res || res <= min)
        continue;
      min = res;
      target_workspace = workspace;
    }
    if(!target_workspace){
      target_workspace = create_workspace(wmgr, type);
      if(!target_workspace){
        fprintf(stderr, "create_workspace failed\n");
        return -1;
      }
    }
    return dpaw_reassign_screen_to_workspace(screen, target_workspace);
  }else{
    return dpaw_reassign_screen_to_workspace(screen, screen->workspace);
  }
  return 0;
}

void dpaw_workspace_screen_cleanup(struct dpaw_workspace_screen* screen){
  dpaw_reassign_screen_to_workspace(screen, 0);
}

void dpaw_workspace_type_register(struct dpaw_workspace_type* type){
  type->window_type->workspace_type = type;
  type->next = workspace_type_list;
  workspace_type_list = type;
}

void dpaw_workspace_type_unregister(struct dpaw_workspace_type* type){
  struct dpaw_workspace_type** pit = 0;
  for(pit=&workspace_type_list; *pit; pit=&(*pit)->next)
    if(*pit == type)
      break;
  if(!*pit){
    fprintf(stderr, "Warning: Couldn't find workspace type object \"%s\" to be unregistred\n", type->name);
    return;
  }
  *pit = type->next;
  type->next = 0;
}

struct dpawindow_app* dpaw_workspace_lookup_xwindow(struct dpaw_workspace* workspace, Window xwindow){
  for(struct dpaw_list_entry* it=workspace->window_list.first; it; it=it->next){
    struct dpawindow_app* app = container_of(it, struct dpawindow_app, workspace_window_entry);
    if(app->window.xwindow == xwindow)
      return app;
  }
  return 0;
}

int dpaw_workspace_remove_window(struct dpawindow_app* window){
  struct dpaw_workspace* workspace = window->workspace;
  if(!workspace)
    return 0;
  if(workspace->type->abandon_window)
    if(workspace->type->abandon_window(window))
      return -1;
  DPAW_APP_UNOBSERVE(window, type);
  DPAW_APP_UNOBSERVE(window, window_hints);
  DPAW_APP_UNOBSERVE(window, desired_placement);
  window->workspace = 0;
  window->workspace_private = 0;
  if(workspace->focus_window == window)
    workspace->focus_window = 0;
  dpaw_linked_list_set(0, &window->workspace_window_entry, 0);
  XReparentWindow(window->window.dpaw->root.display, window->window.xwindow, window->window.dpaw->root.window.xwindow, 0, 0);
  return 0;
}

int dpaw_workspace_add_window(struct dpaw_workspace* workspace, struct dpawindow_app* app_window){
  if(app_window->workspace == workspace)
    return 0;
  if(app_window->workspace)
    if(dpaw_workspace_remove_window(app_window))
      return -1;
  app_window->workspace = workspace;
  app_window->workspace_private = 0;
  dpaw_linked_list_set(&workspace->window_list, &app_window->workspace_window_entry, workspace->window_list.first);
  if(!workspace->type->take_window)
    return -1;
  return workspace->type->take_window(workspace->window, app_window);
}


static void workspace_app_post_cleanup(struct dpawindow* window, void* regptr, void* callptr){
  (void)regptr;
  (void)callptr;
  struct dpawindow_app* app = container_of(window, struct dpawindow_app, window);
  free(app);
}

int dpaw_workspace_manager_manage_window(struct dpaw_workspace_manager* wmgr, Window window, const struct dpaw_workspace_manager_manage_window_options* options){
  {
    struct dpawindow* win = dpawindow_lookup(wmgr->dpaw, window);
    if(win){
      printf("dpaw_workspace_manager_manage_window called, but window was already managed! It's a known %s window\n", win->type->name);
      return 0;
    }
  }
  if(!wmgr->workspace_list.first){
    fprintf(stderr, "Error: No workspaces available\n");
    return -1;
  }
  struct dpaw_workspace* workspace = 0;
  if(options)
    workspace = options->workspace;
  if(!workspace){
    // TODO: add some more logic to this
    workspace = container_of(wmgr->workspace_list.first, struct dpaw_workspace, wmgr_workspace_list_entry);
  }
  struct dpawindow_app* app_window;
  {
    struct dpawindow_workspace_app {
      struct dpawindow_app window;
      // Don't add a pre_cleanup function or other stuff to post_cleanup.
      // They don't cover windows added using dpaw_workspace_add_window.
      // This post_cleanup is just for freeing what we allocate here
      struct dpaw_callback_dpawindow post_cleanup;
    };
    struct dpawindow_workspace_app* wapp = calloc(sizeof(struct dpawindow_workspace_app), 1);
    if(!wapp){
      perror("calloc failed");
      return -1;
    }
    wapp->post_cleanup.callback = workspace_app_post_cleanup;
    app_window = &wapp->window;
    DPAW_CALLBACK_ADD(dpawindow, &app_window->window, post_cleanup, &wapp->post_cleanup);
  }
  if(dpawindow_app_init(wmgr->dpaw, app_window, window))
    return -1;
  if(dpaw_workspace_add_window(workspace, app_window))
    return -1;
  printf("Managing window %lx\n", window);
  return 0;
}


int dpaw_workspace_manager_init(struct dpaw_workspace_manager* wmgr, struct dpaw* dpaw){
  wmgr->dpaw = dpaw;
  if(dpaw_screenchange_listener_register(&wmgr->dpaw->root.screenchange_detector, screenchange_handler, wmgr)){
    fprintf(stderr, "dpaw_screenchange_listener_register failed\n");
    goto error;
  }
  return 0;
error:
  dpaw_workspace_manager_destroy(wmgr);
  return -1;
}

void dpaw_workspace_manager_destroy(struct dpaw_workspace_manager* wmgr){
  if(!wmgr->dpaw)
    return;
  dpaw_screenchange_listener_unregister(&wmgr->dpaw->root.screenchange_detector, screenchange_handler, wmgr);
  while(wmgr->workspace_list.first)
    dpawindow_cleanup(container_of(wmgr->workspace_list.first, struct dpaw_workspace, wmgr_workspace_list_entry)->window);
  wmgr->dpaw = 0;
}
