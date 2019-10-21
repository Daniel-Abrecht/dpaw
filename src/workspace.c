#include <dpawindow/root.h>
#include <workspace.h>
#include <screenchange.h>
#include <stdio.h>
#include <stdlib.h>

static struct dpawin_workspace_type* workspace_type_list;

static void screenchange_handler(void* ptr, enum dpawin_screenchange_type what, const struct dpawin_screen_info* info){
  struct dpawin_workspace_manager* wmgr = ptr;
  printf(
    "Screen %p (%ld-%ld)x(%ld-%ld)\n",
    (void*)info,
    info->boundary.bottom_right.x,
    info->boundary.top_left.x,
    info->boundary.bottom_right.y,
    info->boundary.top_left.y
  );
  switch(what){
    case DPAWIN_SCREENCHANGE_SCREEN_ADDED: {
      struct dpawin_workspace_screen* screen = calloc(sizeof(struct dpawin_workspace_screen), 1);
      if(!screen){
        fprintf(stderr, "Failed to allocate space for dpawin_workspace_screen object\n");
        return;
      }
      dpawin_workspace_screen_init(wmgr, screen, info);
      dpawin_workspace_manager_designate_screen_to_workspace(wmgr, screen);
    } break;
    case DPAWIN_SCREENCHANGE_SCREEN_CHANGED: {
      struct dpawin_workspace_screen* sit = 0;
      for( struct dpawin_workspace* wit = wmgr->workspace; wit; wit=wit->next )
        for( sit = wit->screen; sit; sit=sit->next )
          if( sit->info == info )
            break;
      if(!sit){
        fprintf(stderr, "Warning: dpawin_workspace_screen object which to be updated not found\n");
        return;
      }
      dpawin_workspace_manager_designate_screen_to_workspace(wmgr, sit);
    } break;
    case DPAWIN_SCREENCHANGE_SCREEN_REMOVED: {
      struct dpawin_workspace_screen** psit = (struct dpawin_workspace_screen*[]){0};
      for( struct dpawin_workspace* wit = wmgr->workspace; wit; wit=wit->next )
        for( psit = &wit->screen; psit; psit=&(*psit)->next )
          if( (*psit)->info == info )
            break;
      struct dpawin_workspace_screen* sit = *psit;
      if(!sit){
        fprintf(stderr, "Warning: dpawin_workspace_screen object to be removed not found\n");
        return;
      }
      dpawin_workspace_screen_cleanup(sit);
      *psit = sit->next;
      sit->next = 0;
      free(sit);
    } break;
  }
}

int dpawin_workspace_screen_init(struct dpawin_workspace_manager* wmgr, struct dpawin_workspace_screen* screen, const struct dpawin_screen_info* info){
  (void)wmgr;
  screen->info = info;
  return 0;
}

static int update_workspace(struct dpawin_workspace* workspace){
  if(workspace->screen){
    // Calculate the boundaries just large enough to include all screens
    struct dpawin_rect boundary = workspace->screen->info->boundary;
    for( struct dpawin_workspace_screen* it = workspace->screen->next; it; it=it->next ){
      if( boundary.top_left.x > it->info->boundary.top_left.x )
        boundary.top_left.x = it->info->boundary.top_left.x;
      if( boundary.top_left.y > it->info->boundary.top_left.y )
        boundary.top_left.y = it->info->boundary.top_left.y;
      if( boundary.bottom_right.x < it->info->boundary.bottom_right.x )
        boundary.bottom_right.x = it->info->boundary.bottom_right.x;
      if( boundary.bottom_right.y < it->info->boundary.bottom_right.y )
        boundary.bottom_right.y = it->info->boundary.bottom_right.y;
    }
  }
  return 0;
}

int dpawin_reassign_screen_to_workspace(struct dpawin_workspace_screen* screen, struct dpawin_workspace* workspace){
  if(screen->workspace == workspace){
    if(!workspace)
      return 0;
    update_workspace(workspace);
    if(workspace->type->screen_changed)
      return workspace->type->screen_changed(workspace->window, screen);
    return 0;
  }
  if(screen->workspace){
    struct dpawin_workspace* old_workspace = screen->workspace;
    for( struct dpawin_workspace_screen** psit = &workspace->screen; psit; psit=&(*psit)->next ){
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
      if(workspace->type->screen_added(workspace->window, screen) != 0)
        return -1;
  }
  return 0;
}

struct dpawin_workspace_type* choose_best_target_workspace_type(struct dpawin_workspace_manager* wmgr, struct dpawin_workspace_screen* screen){
   (void)wmgr;
   (void)screen;
  // TODO: Come up with properties to determine the most fitting workspace type.
  // For now, let's just take a random one and worry about this later.
  return workspace_type_list;
}

struct dpawin_workspace* create_workspace(struct dpawin_workspace_manager* wmgr, struct dpawin_workspace_type* type){
  if(type->size+type->derived_offset < sizeof(struct dpawin_workspace)){
    fprintf(stderr, "Error: Invalid workspace type size\n");
    return 0;
  }
  void* memory = calloc(type->size, 1);
  if(!memory){
    fprintf(stderr, "calloc failed\n");
    goto error;
  }
  struct dpawin_workspace* workspace = (struct dpawin_workspace*)((char*)memory+type->derived_offset);
  workspace->type = type;
  workspace->workspace_manager = wmgr;
  workspace->window = memory;

  int xscreen = DefaultScreen(wmgr->root->display);
  Visual* visual = DefaultVisual(wmgr->root->display, xscreen);
  int depth  = DefaultDepth(wmgr->root->display, xscreen);

  Window window = XCreateWindow(
    wmgr->root->display, wmgr->root->window.xwindow,
    0, 0, 0, 0, 0,
    depth,  InputOutput,
    visual, 0, 0
  );
  if(!window){
    fprintf(stderr, "XCreateWindow failed\n");
    goto error;
  }
  workspace->window->xwindow = window;

  workspace->type->init_window_super(workspace->window);
  if(type->init)
    if(workspace->type->init(workspace->window) != 0)
      goto error;

  return workspace;

error:
  if(memory)
    free(memory);
  return 0;
}

int dpawin_workspace_manager_designate_screen_to_workspace(struct dpawin_workspace_manager* wmgr, struct dpawin_workspace_screen* screen){
  if(!screen->workspace){
    struct dpawin_workspace_type* type = choose_best_target_workspace_type(wmgr, screen);
    if(!type){
      fprintf(stderr, "Error: choose_best_target_workspace_type failed\n");
      return -1;
    }
    // Does any existing workspace of the desired type want this screen?
    struct dpawin_workspace* target_workspace = 0;
    int min = 0;
    for(struct dpawin_workspace* it=wmgr->workspace; it; it=it->next){
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
    return dpawin_reassign_screen_to_workspace(screen, target_workspace);
  }
  return 0;
}

void dpawin_workspace_screen_cleanup(struct dpawin_workspace_screen* screen){
  dpawin_reassign_screen_to_workspace(screen, 0);
}

void dpawin_workspace_type_register(struct dpawin_workspace_type* type){
  type->next = workspace_type_list;
  workspace_type_list = type;
}

void dpawin_workspace_type_unregister(struct dpawin_workspace_type* type){
  struct dpawin_workspace_type** pit = 0;
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

int dpawin_workspace_manager_init(struct dpawin_workspace_manager* wmgr, struct dpawindow_root* root){
  wmgr->root = root;
  if(dpawin_screenchange_check() == -1){
    fprintf(stderr, "dpawin_screenchange_check failed\n");
    goto error;
  }
  if(dpawin_screenchange_listener_register(screenchange_handler, wmgr) == -1){
    fprintf(stderr, "dpawin_screenchange_listener_register failed\n");
    goto error;
  }
  return 0;
error:
  dpawin_workspace_manager_destroy(wmgr);
  return -1;
}

void dpawin_workspace_manager_destroy(struct dpawin_workspace_manager* wmgr){
  dpawin_screenchange_listener_unregister(screenchange_handler, wmgr);
  wmgr->root = 0;
}
