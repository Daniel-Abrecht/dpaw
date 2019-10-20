#include <screenchange.h>
#include <dpawin.h>
#include <X11/extensions/Xinerama.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

struct screenchange_listener {
  struct screenchange_listener* next;
  dpawin_screenchange_handler_t callback;
  void* ptr;
};

static struct screenchange_listener* screenchange_listener_list;

struct dpawin_screen_info_list_entry {
  struct dpawin_screen_info info;
  int id;
  bool lost;
  struct dpawin_screen_info_list_entry* next;
};
static struct dpawin_screen_info_list_entry* screen_list;

static void invalidate_screen_info(void){
  for(struct dpawin_screen_info_list_entry* it=screen_list; it; it=it->next)
    it->lost = true;
}

static int commit_screen_info(int id, struct dpawin_rect boundary){
  struct dpawin_screen_info_list_entry* it;
  for(it=screen_list; it; it=it->next){
    if(id != -1){
      if(it->id != id)
        continue;
    }else if(it->lost || it->info.boundary.position.x != boundary.position.x || it->info.boundary.position.y != boundary.position.y)
      continue;
    break;
  }
  if(it){
    it->lost = false;
    if( it->info.boundary.position.x != boundary.position.x || it->info.boundary.position.y != boundary.position.y
    || it->info.boundary.size.x     != boundary.size.x     || it->info.boundary.size.y     != boundary.size.y
    ) for(struct screenchange_listener* it2=screenchange_listener_list; it2; it2=it2->next)
        it2->callback(it2->ptr, DPAWIN_SCREENCHANGE_SCREEN_CHANGED, &it->info);
  }else{
    struct dpawin_screen_info_list_entry* info = calloc(sizeof(struct dpawin_screen_info_list_entry), 1);
    if(!info)
      return -1;
    info->next = screen_list;
    info->id = id;
    info->info.boundary = boundary;
    screen_list = info;
    for(struct screenchange_listener* it2=screenchange_listener_list; it2; it2=it2->next)
      it2->callback(it2->ptr, DPAWIN_SCREENCHANGE_SCREEN_ADDED, &info->info);
  }
  return 0;
}

static void finalize_screen_info(void){
  for(struct dpawin_screen_info_list_entry* it=screen_list; it; it=it->next){
    if(!it->lost)
      continue;
    for(struct screenchange_listener* it2=screenchange_listener_list; it2; it2=it2->next)
      it2->callback(it2->ptr, DPAWIN_SCREENCHANGE_SCREEN_REMOVED, &it->info);
    free(it);
  }
}

static int check_screens_xinerama(void){
  int event_base_return, error_base_return;
  if(!XineramaQueryExtension(dpawin.root.display, &event_base_return, &error_base_return)){
    fprintf(stderr, "Warning: XineramaQueryExtension failed\n");
    return -1;
  }
  if(!XineramaIsActive(dpawin.root.display)){
    fprintf(stderr, "Warning: Xinerama isn't active\n");
    return -1;
  }
  int screen_count = 0;
  XineramaScreenInfo *info = XineramaQueryScreens(dpawin.root.display, &screen_count);
  if(!info){
    fprintf(stderr, "Warning: XineramaQueryScreens failed\n");
    return -1;
  }
  if(!screen_count){
    fprintf(stderr, "Error: XineramaQueryScreens failed in a way it's not supposed to\n");
    XFree(info);
    return -1;
  }
  invalidate_screen_info();
  for(int i=0; i<screen_count; i++)
    commit_screen_info(info[i].screen_number, (struct dpawin_rect){
      .size = {
        .x  = info[i].width,
        .y = info[i].height
      },
      .position = {
        .x  = info[i].x_org,
        .y  = info[i].y_org
      }
    });
  finalize_screen_info();
  XFree(info);
  return 0;
}

int dpawin_screenchange_check(void){
  return check_screens_xinerama();
}

int dpawin_screenchange_listener_register(dpawin_screenchange_handler_t callback, void* ptr){
  struct screenchange_listener* scl = calloc(sizeof(struct screenchange_listener), 1);
  if(!scl){
    fprintf(stderr, "Error: Failed to allocate space for screnn change listener.\n");
    return -1;
  }
  scl->callback = callback;
  scl->ptr = ptr;

  scl->next = screenchange_listener_list;
  screenchange_listener_list = scl;

  for(struct dpawin_screen_info_list_entry* it=screen_list; it; it=it->next)
    callback(ptr, DPAWIN_SCREENCHANGE_SCREEN_ADDED, &it->info);

  return 0;
}

int dpawin_screenchange_listener_unregister(dpawin_screenchange_handler_t callback, void* ptr){
  struct screenchange_listener** pit = &screenchange_listener_list;
  for( ; *pit; pit=&(*pit)->next ){
    struct screenchange_listener* it = *pit;
    if(it->callback != callback || (ptr && it->ptr != ptr))
      continue;
    *pit = it->next;
    it->next = 0;
    return 0;
  }
  fprintf(stderr, "Warning: Failed to find screen change listener to be removed.\n");
  return -1;
}
