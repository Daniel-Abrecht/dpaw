#include <dpawindow/root.h>
#include <dpawindow/app.h>
#include <workspace.h>
#include <dpaw.h>
#include <screenchange.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

static struct dpaw_workspace_type* workspace_type_list;

static void screenchange_handler(void* ptr, enum dpaw_screenchange_type what, const struct dpaw_screen_info* info){
  struct dpaw_workspace_manager* wmgr = ptr;
  printf(
    "Screen %p (%ld-%ld)x(%ld-%ld)\n",
    (void*)info,
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
      struct dpaw_workspace_screen* sit = 0;
      for( struct dpaw_workspace* wit = wmgr->workspace; wit; wit=wit->next )
        for( sit = wit->screen; sit; sit=sit->next )
          if( sit->info == info )
            break;
      if(!sit){
        fprintf(stderr, "Warning: dpaw_workspace_screen object which to be updated not found\n");
        return;
      }
      dpaw_workspace_manager_designate_screen_to_workspace(wmgr, sit);
    } break;
    case DPAW_SCREENCHANGE_SCREEN_REMOVED: {
      struct dpaw_workspace_screen** psit = (struct dpaw_workspace_screen*[]){0};
      for( struct dpaw_workspace* wit = wmgr->workspace; wit; wit=wit->next )
        for( psit = &wit->screen; psit; psit=&(*psit)->next )
          if( (*psit)->info == info )
            break;
      struct dpaw_workspace_screen* sit = *psit;
      if(!sit){
        fprintf(stderr, "Warning: dpaw_workspace_screen object to be removed not found\n");
        return;
      }
      dpaw_workspace_screen_cleanup(sit);
      *psit = sit->next;
      sit->next = 0;
      free(sit);
    } break;
  }
}

int dpaw_workspace_screen_init(struct dpaw_workspace_manager* wmgr, struct dpaw_workspace_screen* screen, const struct dpaw_screen_info* info){
  (void)wmgr;
  screen->info = info;
  return 0;
}

static int update_workspace(struct dpaw_workspace* workspace){
  if(workspace->screen){
    // Calculate the boundaries just large enough to include all screens
    struct dpaw_rect boundary = workspace->screen->info->boundary;
    for( struct dpaw_workspace_screen* it = workspace->screen->next; it; it=it->next ){
      if( boundary.top_left.x > it->info->boundary.top_left.x )
        boundary.top_left.x = it->info->boundary.top_left.x;
      if( boundary.top_left.y > it->info->boundary.top_left.y )
        boundary.top_left.y = it->info->boundary.top_left.y;
      if( boundary.bottom_right.x < it->info->boundary.bottom_right.x )
        boundary.bottom_right.x = it->info->boundary.bottom_right.x;
      if( boundary.bottom_right.y < it->info->boundary.bottom_right.y )
        boundary.bottom_right.y = it->info->boundary.bottom_right.y;
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
    for( struct dpaw_workspace_screen** psit = &workspace->screen; psit; psit=&(*psit)->next ){
      if( *psit != screen )
        continue;
      *psit = 0;
      break;
    }
    screen->workspace = 0;
    update_workspace(old_workspace);
    if(old_workspace->type->screen_removed)
      old_workspace->type->screen_removed(old_workspace->window, screen);
  }
  if(workspace){
    screen->workspace = workspace;
    screen->next = workspace->screen;
    workspace->screen = screen;
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

struct dpaw_workspace* create_workspace(struct dpaw_workspace_manager* wmgr, struct dpaw_workspace_type* type){
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

  Window window = XCreateWindow(
    wmgr->dpaw->root.display, wmgr->dpaw->root.window.xwindow,
    0, 0, 1, 1, 0,
    CopyFromParent,  InputOutput,
    CopyFromParent, 0, 0
  );
  if(!window){
    fprintf(stderr, "XCreateWindow failed\n");
    goto error;
  }
  workspace->window->xwindow = window;

  if(workspace->type->init_window_super(wmgr->dpaw, workspace->window)){
    fprintf(stderr, "%s::init_window_super failed\n", workspace->type->name);
    goto error;
  }

  if(type->init){
    if(workspace->type->init(workspace->window)){
      fprintf(stderr, "%s::init failed\n", workspace->type->name);
      goto error;
    }
  }

  dpawindow_set_mapping(workspace->window, true);

  workspace->next = wmgr->workspace;
  wmgr->workspace = workspace;

  return workspace;

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
    for(struct dpaw_workspace* it=wmgr->workspace; it; it=it->next){
      if(it->type != type)
        continue;
      if(!it->type->screen_make_bid)
        continue;
      int res = it->type->screen_make_bid(it->window, screen);
      if(!res || res <= min)
        continue;
      min = res;
      target_workspace = it;
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
  window->workspace = 0;
  window->workspace_private = 0;
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

int dpaw_workspace_manager_manage_window(struct dpaw_workspace_manager* wmgr, Window window){
  struct dpaw_workspace* workspace = wmgr->workspace;
  if(!workspace){
    fprintf(stderr, "Error: No workspaces available\n");
    return -1;
  }
  struct dpawindow_app* app_window = calloc(sizeof(struct dpawindow_app), 1);
  if(!app_window){
    perror("calloc failed");
    return -1;
  }
  if(dpawindow_app_init(wmgr->dpaw, app_window, window))
    return -1;
  return dpaw_workspace_add_window(workspace, app_window);
}

int dpaw_workspace_manager_abandon_window(struct dpawindow_app* window){
  if(window->workspace)
    dpaw_workspace_remove_window(window);
  if(dpawindow_app_cleanup(window))
    return -1;
  free(window);
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
  // TODO: Reparent all windows to root & remove all workspaces
  dpaw_screenchange_listener_unregister(&wmgr->dpaw->root.screenchange_detector, screenchange_handler, wmgr);
  wmgr->dpaw = 0;
}
