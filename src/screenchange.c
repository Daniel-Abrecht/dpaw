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
  struct dpaw_screen_info_list_entry* next;
};

int commit_screen_info(struct dpaw_screenchange_detector* detector, unsigned long id, const struct dpaw_screen_info* info){
  struct dpaw_rect boundary = info->boundary;
  if( boundary.top_left.x > boundary.bottom_right.x
   || boundary.top_left.y > boundary.bottom_right.y
  ) memset(&boundary, 0, sizeof(boundary));
  struct dpaw_screen_info_list_entry* it;
  for(it=detector->screen_list; it; it=it->next)
    if(it->id == id)
      break;
  if(it){
    bool changed = (
        it->info.boundary.top_left.x != boundary.top_left.x
     || it->info.boundary.top_left.y != boundary.top_left.y
     || it->info.boundary.bottom_right.x != boundary.bottom_right.x
     || it->info.boundary.bottom_right.y != boundary.bottom_right.y
    );
    it->info = *info;
    it->info.boundary = boundary;
    if(changed){
      for(struct screenchange_listener* it2=detector->screenchange_listener_list; it2; it2=it2->next)
        it2->callback(it2->ptr, DPAW_SCREENCHANGE_SCREEN_CHANGED, &it->info);
    }
  }else{
    struct dpaw_screen_info_list_entry* info_entry = calloc(sizeof(struct dpaw_screen_info_list_entry), 1);
    if(!info_entry)
      return -1;
    info_entry->next = detector->screen_list;
    info_entry->id = id;
    info_entry->info = *info;
    info_entry->info.boundary = boundary;
    detector->screen_list = info_entry;
    for(struct screenchange_listener* it2=detector->screenchange_listener_list; it2; it2=it2->next)
      it2->callback(it2->ptr, DPAW_SCREENCHANGE_SCREEN_ADDED, &info_entry->info);
  }
  return 0;
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

static int randr_add_or_update_output(struct dpaw_screenchange_detector* detector, XID outputid){
  struct dpaw_xrandr_private* xrandr = detector->xrandr;
  int ret = -1;
  struct dpaw_xrandr_output output;
  output.name.id = outputid;
  XRROutputInfo* output_info = XRRGetOutputInfo(detector->dpaw->root.display, xrandr->screen_resources, output.name.id);
  if(!output_info){
    fprintf(stderr, "XRRGetOutputInfo failed for output %lx\n", (long)output.name.id);
    goto error;
  }
  if(output_info->connection != RR_Connected){
    ret = 0;
    goto error_after_XRRGetOutputInfo;
  }
  XRRCrtcInfo* crtc_info = XRRGetCrtcInfo(detector->dpaw->root.display, xrandr->screen_resources, output_info->crtc);
  if(!crtc_info){
    fprintf(stderr, "XRRFreeOutputInfo failed for crtc %lx\n", (long)output_info->crtc);
    goto error_after_XRRGetOutputInfo;
  }
  output.name.string = strdup(output_info->name);
  if(!output.name.string){
    perror("strdup failed");
    goto error_after_XRRGetCrtcInfo;
  }
  struct dpaw_screen_info info = {
    .name = output.name.string,
    .boundary = {
      .top_left = {
        .x = crtc_info->x,
        .y = crtc_info->y,
      },
      .bottom_right = {
        .x = crtc_info->x + crtc_info->width,
        .y = crtc_info->y + crtc_info->height,
      }
    },
    .physical_size_mm = {
      .x = output_info->mm_width,
      .y = output_info->mm_height,
    },
  };
  ret = commit_screen_info(detector, output.name.id, &info);

error_after_XRRGetCrtcInfo:
  XRRFreeCrtcInfo(crtc_info);
error_after_XRRGetOutputInfo:
  XRRFreeOutputInfo(output_info);
error:
  return ret;
}

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

  for(int i=0; i < xrandr->screen_resources->noutput; i++)
    if(randr_add_or_update_output(detector, xrandr->screen_resources->outputs[i]) != 0)
      fprintf(stderr, "randr_add_or_update_output failed\n");

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
  while(detector->screenchange_listener_list)
    dpaw_screenchange_listener_unregister(detector, detector->screenchange_listener_list->callback, 0);
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
    free(it);
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
  randr_add_or_update_output(&window->screenchange_detector, event->output);
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
