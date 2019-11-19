#include <xev/randr.c>
#include <dpawindow/root.h>
#include <screenchange.h>
#include <X11/extensions/Xinerama.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

struct screenchange_listener {
  struct screenchange_listener* next;
  dpawin_screenchange_handler_t callback;
  void* ptr;
};

struct dpawin_screen_info_list_entry {
  struct dpawin_screen_info info;
  int id;
  bool lost;
  struct dpawin_screen_info_list_entry* next;
};

static void invalidate_screen_info(struct dpawin_screenchange_detector* detector){
  for(struct dpawin_screen_info_list_entry* it=detector->screen_list; it; it=it->next)
    it->lost = true;
}

static int commit_screen_info(struct dpawin_screenchange_detector* detector, int id, struct dpawin_rect boundary){
  if( boundary.top_left.x > boundary.bottom_right.x
   || boundary.top_left.y > boundary.bottom_right.y
  ) memset(&boundary, 0, sizeof(boundary));
  struct dpawin_screen_info_list_entry* it;
  for(it=detector->screen_list; it; it=it->next){
    if(id != -1){
      if(it->id != id)
        continue;
    }else if(it->lost || it->info.boundary.top_left.x != boundary.top_left.x || it->info.boundary.top_left.y != boundary.top_left.y)
      continue;
    break;
  }
  if(it){
    it->lost = false;
    if( it->info.boundary.top_left.x != boundary.top_left.x
     || it->info.boundary.top_left.y != boundary.top_left.y
     || it->info.boundary.bottom_right.x != boundary.bottom_right.x
     || it->info.boundary.bottom_right.y != boundary.bottom_right.y
    ) for(struct screenchange_listener* it2=detector->screenchange_listener_list; it2; it2=it2->next)
        it2->callback(it2->ptr, DPAWIN_SCREENCHANGE_SCREEN_CHANGED, &it->info);
  }else{
    struct dpawin_screen_info_list_entry* info = calloc(sizeof(struct dpawin_screen_info_list_entry), 1);
    if(!info)
      return -1;
    info->next = detector->screen_list;
    info->id = id;
    info->info.boundary = boundary;
    detector->screen_list = info;
    for(struct screenchange_listener* it2=detector->screenchange_listener_list; it2; it2=it2->next)
      it2->callback(it2->ptr, DPAWIN_SCREENCHANGE_SCREEN_ADDED, &info->info);
  }
  return 0;
}

static void finalize_screen_info(struct dpawin_screenchange_detector* detector){
  for(struct dpawin_screen_info_list_entry* it=detector->screen_list; it; it=it->next){
    if(!it->lost)
      continue;
    for(struct screenchange_listener* it2=detector->screenchange_listener_list; it2; it2=it2->next)
      it2->callback(it2->ptr, DPAWIN_SCREENCHANGE_SCREEN_REMOVED, &it->info);
    free(it);
  }
}

static int xinerama_init(struct dpawin_screenchange_detector* detector){
  int event_base_return, error_base_return;
  if(!XineramaQueryExtension(detector->display, &event_base_return, &error_base_return)){
    fprintf(stderr, "Warning: XineramaQueryExtension failed\n");
    return -1;
  }
  if(!XineramaIsActive(detector->display)){
    fprintf(stderr, "Warning: Xinerama isn't active\n");
    return -1;
  }
  int screen_count = 0;
  XineramaScreenInfo *info = XineramaQueryScreens(detector->display, &screen_count);
  if(!info){
    fprintf(stderr, "Warning: XineramaQueryScreens failed\n");
    return -1;
  }
  if(!screen_count){
    fprintf(stderr, "Error: XineramaQueryScreens failed in a way it's not supposed to\n");
    XFree(info);
    return -1;
  }
  invalidate_screen_info(detector);
  for(int i=0; i<screen_count; i++)
    commit_screen_info(detector, info[i].screen_number, (struct dpawin_rect){
      .top_left = {
        .x  = info[i].x_org,
        .y  = info[i].y_org
      },
      .bottom_right = {
        .x  = (long)info[i].x_org + info[i].width,
        .y  = (long)info[i].y_org + info[i].height
      }
    });
  finalize_screen_info(detector);
  XFree(info);
  return 0;
}

int dpawin_screenchange_init(struct dpawin_screenchange_detector* detector, Display* display){
  detector->display = display;
  return xinerama_init(detector);
}

EV_ON(root, RRScreenChangeNotify){
  (void)window;
  (void)event;
  puts("RRScreenChangeNotify");
  return EHR_OK;
}

int dpawin_screenchange_listener_register(struct dpawin_screenchange_detector* detector, dpawin_screenchange_handler_t callback, void* ptr){
  struct screenchange_listener* scl = calloc(sizeof(struct screenchange_listener), 1);
  if(!scl){
    fprintf(stderr, "Error: Failed to allocate space for screnn change listener.\n");
    return -1;
  }
  scl->callback = callback;
  scl->ptr = ptr;

  scl->next = detector->screenchange_listener_list;
  detector->screenchange_listener_list = scl;

  for(struct dpawin_screen_info_list_entry* it=detector->screen_list; it; it=it->next)
    callback(ptr, DPAWIN_SCREENCHANGE_SCREEN_ADDED, &it->info);

  return 0;
}

int dpawin_screenchange_listener_unregister(struct dpawin_screenchange_detector* detector, dpawin_screenchange_handler_t callback, void* ptr){
  struct screenchange_listener** pit = &detector->screenchange_listener_list;
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
