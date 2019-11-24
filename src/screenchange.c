#include <dpawindow/root.h>
#include <screenchange.h>
#include <xev/randr.c>
#include <dpaw.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

struct screenchange_listener {
  struct screenchange_listener* next;
  dpaw_screenchange_handler_t callback;
  void* ptr;
};

struct dpaw_screen_info_list_entry {
  struct dpaw_screen_info info;
  unsigned long id;
  bool lost;
  struct dpaw_screen_info_list_entry* next;
};

static void invalidate_screen_info(struct dpaw_screenchange_detector* detector){
  for(struct dpaw_screen_info_list_entry* it=detector->screen_list; it; it=it->next)
    it->lost = true;
}

int commit_screen_info(struct dpaw_screenchange_detector* detector, unsigned long id, const char* name, struct dpaw_rect boundary){
  if( boundary.top_left.x > boundary.bottom_right.x
   || boundary.top_left.y > boundary.bottom_right.y
  ) memset(&boundary, 0, sizeof(boundary));
  struct dpaw_screen_info_list_entry* it;
  for(it=detector->screen_list; it; it=it->next){
    if(!id){
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
        it2->callback(it2->ptr, DPAW_SCREENCHANGE_SCREEN_CHANGED, &it->info);
  }else{
    struct dpaw_screen_info_list_entry* info = calloc(sizeof(struct dpaw_screen_info_list_entry), 1);
    if(!info)
      return -1;
    info->next = detector->screen_list;
    info->id = id;
    info->info.name = name;
    info->info.boundary = boundary;
    detector->screen_list = info;
    for(struct screenchange_listener* it2=detector->screenchange_listener_list; it2; it2=it2->next)
      it2->callback(it2->ptr, DPAW_SCREENCHANGE_SCREEN_ADDED, &info->info);
  }
  return 0;
}

static void finalize_screen_info(struct dpaw_screenchange_detector* detector){
  for(struct dpaw_screen_info_list_entry* it=detector->screen_list; it; it=it->next){
    if(!it->lost)
      continue;
    for(struct screenchange_listener* it2=detector->screenchange_listener_list; it2; it2=it2->next)
      it2->callback(it2->ptr, DPAW_SCREENCHANGE_SCREEN_REMOVED, &it->info);
    free(it);
  }
}

struct dpaw_xrandr_name {
  char* string;
  XID id;
};

struct dpaw_xrandr_output {
  struct dpaw_xrandr_name name;
};

struct dpaw_xrandr_private {
  XRRScreenResources* screen_resources;
};

static int randr_init(struct dpaw_screenchange_detector* detector){
  struct dpaw_xrandr_private* xrandr = calloc(sizeof(*xrandr), 1);
  if(!xrandr){
    perror("calloc failed");
    return -1;
  }
  detector->xrandr = xrandr;
  xrandr->screen_resources = XRRGetScreenResourcesCurrent(detector->dpaw->root.display, detector->dpaw->root.window.xwindow);
  if(!xrandr->screen_resources){
    fprintf(stderr, "XRRGetScreenResourcesCurrent failed");
    return -1;
  }

  invalidate_screen_info(detector);
  for(int i=0; i < xrandr->screen_resources->noutput; i++){
    struct dpaw_xrandr_output output;
    output.name.id = xrandr->screen_resources->outputs[i];
    XRROutputInfo* output_info = XRRGetOutputInfo(detector->dpaw->root.display, xrandr->screen_resources, output.name.id);
    if(!output_info){
      fprintf(stderr, "XRRGetOutputInfo failed for output %lx\n", (long)output.name.id);
      continue;
    }
    if(output_info->connection != RR_Connected){
      XRRFreeOutputInfo(output_info);
      continue;
    }
    XRRCrtcInfo* crtc_info = XRRGetCrtcInfo(detector->dpaw->root.display, xrandr->screen_resources, output_info->crtc);
    if(!crtc_info){
      fprintf(stderr, "XRRFreeOutputInfo failed for crtc %lx\n", (long)output_info->crtc);
      XRRFreeOutputInfo(output_info);
      continue;
    }
    output.name.string = strdup(output_info->name);
    if(!output.name.string){
      perror("strdup failed");
      continue;
    }
/*    output_info->mm_width,
    output_info->mm_height,*/
    commit_screen_info(detector, output.name.id, output.name.string, (struct dpaw_rect){
      .top_left = {
        .x = crtc_info->x,
        .y = crtc_info->y,
      },
      .bottom_right = {
        .x = crtc_info->x + crtc_info->width,
        .y = crtc_info->y + crtc_info->height,
      }
    });
    XRRFreeCrtcInfo(crtc_info);
    XRRFreeOutputInfo(output_info);
  }
  finalize_screen_info(detector);
  return 0;
}

static void randr_destroy(struct dpaw_screenchange_detector* detector){
  if(!detector->xrandr)
    return;
  XRRFreeScreenResources(detector->xrandr->screen_resources);
  free(detector->xrandr);
  detector->xrandr = 0;
}

int dpaw_screenchange_init(struct dpaw_screenchange_detector* detector, struct dpaw* dpaw){
  detector->dpaw = dpaw;
  return randr_init(detector);
}

void dpaw_screenchange_destroy(struct dpaw_screenchange_detector* detector){
  randr_destroy(detector);
}

int dpaw_screenchange_listener_register(struct dpaw_screenchange_detector* detector, dpaw_screenchange_handler_t callback, void* ptr){
  struct screenchange_listener* scl = calloc(sizeof(struct screenchange_listener), 1);
  if(!scl){
    fprintf(stderr, "Error: Failed to allocate space for screnn change listener.\n");
    return -1;
  }
  scl->callback = callback;
  scl->ptr = ptr;

  scl->next = detector->screenchange_listener_list;
  detector->screenchange_listener_list = scl;

  for(struct dpaw_screen_info_list_entry* it=detector->screen_list; it; it=it->next)
    callback(ptr, DPAW_SCREENCHANGE_SCREEN_ADDED, &it->info);

  return 0;
}

int dpaw_screenchange_listener_unregister(struct dpaw_screenchange_detector* detector, dpaw_screenchange_handler_t callback, void* ptr){
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

EV_ON(root, RRScreenChangeNotify){
  (void)window;
  (void)event;
  puts("RRScreenChangeNotify");
  return EHR_OK;
}

EV_ON(root, RRNotify_CrtcChange){
  (void)window;
  (void)event;
  puts("RRNotify_CrtcChange");
  return EHR_OK;
}

EV_ON(root, RRNotify_OutputChange){
  (void)window;
  (void)event;
  puts("RRNotify_OutputChange");
  return EHR_OK;
}

EV_ON(root, RRNotify_OutputProperty){
  (void)window;
  (void)event;
  puts("RRNotify_OutputProperty");
  return EHR_OK;
}

EV_ON(root, RRNotify_ProviderChange){
  (void)window;
  (void)event;
  puts("RRNotify_ProviderChange");
  return EHR_OK;
}

EV_ON(root, RRNotify_ProviderProperty){
  (void)window;
  (void)event;
  puts("RRNotify_ProviderProperty");
  return EHR_OK;
}

EV_ON(root, RRNotify_ResourceChange){
  (void)window;
  (void)event;
  puts("RRNotify_ResourceChange");
  return EHR_OK;
}
