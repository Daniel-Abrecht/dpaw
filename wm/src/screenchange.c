#include <-dpaw/dpaw.h>
#include <-dpaw/xev/randr.c>
#include <-dpaw/screenchange.h>
#include <-dpaw/dpawindow/root.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

enum screenchange_detector_source {
  SCREENCHANGE_DETECTOR_XRANDR
};

struct screenchange_listener {
  struct dpaw_list_entry screenchange_detector_listener_entry;
  dpaw_screenchange_handler_t callback;
  void* ptr;
};

struct dpaw_screen_info_list_entry {
  union { // must be the first entry
    struct dpaw_list_entry screen_entry; // This makes the address of screen_entry, dpaw_screen_info_list_entry and dpaw_screen_info the same, thus allowing casting between them. Removing this would require quiet a few more container_of usages, which isn't pretty either.
    struct dpaw_screen_info info;
  };
  enum screenchange_detector_source source;
  unsigned long id;
};

void remove_screen_info(struct dpaw_screenchange_detector* detector, struct dpaw_screen_info_list_entry* entry){
  (void)detector;
  if(entry->info.name)
    free(entry->info.name);
  entry->info.name = 0;
  dpaw_linked_list_set(0, &entry->screen_entry, 0);
  free(entry);
}

static void copy_screen_info(struct dpaw_screen_info* dst, const struct dpaw_screen_info* src){
  // The list mustn't be changed, and the list con't simply be copied like that
  struct dpaw_list_entry screen_entry = dst->screen_entry;
  *dst = *src;
  dst->screen_entry = screen_entry;
}

int update_screen_info(struct dpaw_screenchange_detector* detector, unsigned long id, const struct dpaw_screen_info* info, enum screenchange_detector_source source){
  struct dpaw_rect boundary = info->boundary;
  if( boundary.top_left.x > boundary.bottom_right.x
   || boundary.top_left.y > boundary.bottom_right.y
  ) memset(&boundary, 0, sizeof(boundary));
  struct dpaw_screen_info_list_entry* si = 0;
  for(struct dpaw_list_entry* it=detector->screen_list.first; it; it=it->next){
    struct dpaw_screen_info_list_entry* entry = container_of(it, struct dpaw_screen_info_list_entry, screen_entry);
    if(entry->id != id)
      continue;
    si = entry;
    break;
  }
  if(si){
    bool changed = (
        si->info.boundary.top_left.x != boundary.top_left.x
     || si->info.boundary.top_left.y != boundary.top_left.y
     || si->info.boundary.bottom_right.x != boundary.bottom_right.x
     || si->info.boundary.bottom_right.y != boundary.bottom_right.y
    );
    if(si->info.name)
      free(si->info.name);
    copy_screen_info(&si->info, info);
    if(si->info.name)
      si->info.name = strdup(si->info.name);
    si->info.boundary = boundary;
    if(changed){
      for(struct dpaw_list_entry* it=detector->screenchange_listener_list.first; it; it=it->next){
        struct screenchange_listener* listener = container_of(it, struct screenchange_listener, screenchange_detector_listener_entry);
        listener->callback(listener->ptr, DPAW_SCREENCHANGE_SCREEN_CHANGED, &si->info);
      }
    }
  }else{
    si = calloc(sizeof(struct dpaw_screen_info_list_entry), 1);
    if(!si)
      return -1;
    si->source = source;
    dpaw_linked_list_set(&detector->screen_list, &si->screen_entry, 0);
    si->id = id;
    copy_screen_info(&si->info, info);
    if(si->info.name)
      si->info.name = strdup(si->info.name);
    si->info.boundary = boundary;
    for(struct dpaw_list_entry* it=detector->screenchange_listener_list.first; it; it=it->next){
      struct screenchange_listener* listener = container_of(it, struct screenchange_listener, screenchange_detector_listener_entry);
      listener->callback(listener->ptr, DPAW_SCREENCHANGE_SCREEN_ADDED, &si->info);
    }
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
  XRROutputInfo* output_info = XRRGetOutputInfo(detector->dpaw->root.display, xrandr->screen_resources, outputid);
  if(!output_info){
    fprintf(stderr, "XRRGetOutputInfo failed for output %lx\n", (long)outputid);
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
  struct dpaw_screen_info info = {
    .name = output_info->name,
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
  ret = update_screen_info(detector, outputid, &info, SCREENCHANGE_DETECTOR_XRANDR);

//error_after_XRRGetCrtcInfo:
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
  for(struct dpaw_list_entry *it=detector->screen_list.first, *next; it; it=next){
    next = it->next;
    struct dpaw_screen_info_list_entry* entry = container_of(it, struct dpaw_screen_info_list_entry, screen_entry);
    if(entry->source != SCREENCHANGE_DETECTOR_XRANDR)
      continue;
    remove_screen_info(detector, entry);
  }
  XRRFreeScreenResources(detector->xrandr->screen_resources);
  free(detector->xrandr);
  detector->xrandr = 0;
}

int dpaw_screenchange_init(struct dpaw_screenchange_detector* detector, struct dpaw* dpaw){
  detector->dpaw = dpaw;
  return randr_init(detector);
}

void dpaw_screenchange_destroy(struct dpaw_screenchange_detector* detector){
  while(detector->screenchange_listener_list.first)
    dpaw_screenchange_listener_unregister(detector, container_of(detector->screenchange_listener_list.first, struct screenchange_listener, screenchange_detector_listener_entry)->callback, 0);
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

  dpaw_linked_list_set(&detector->screenchange_listener_list, &scl->screenchange_detector_listener_entry, 0);

  for(struct dpaw_list_entry* it=detector->screen_list.first; it; it=it->next)
    callback(ptr, DPAW_SCREENCHANGE_SCREEN_ADDED, &container_of(it, struct dpaw_screen_info_list_entry, screen_entry)->info);

  return 0;
}

int dpaw_screenchange_listener_unregister(struct dpaw_screenchange_detector* detector, dpaw_screenchange_handler_t callback, void* ptr){
  for(struct dpaw_list_entry* it=detector->screenchange_listener_list.first; it; it=it->next){
    struct screenchange_listener* listener = container_of(it, struct screenchange_listener, screenchange_detector_listener_entry);
    if(listener->callback != callback || (ptr && listener->ptr != ptr))
      continue;
    dpaw_linked_list_set(0, it, 0);
    for(struct dpaw_list_entry* it=detector->screen_list.first; it; it=it->next)
      callback(ptr, DPAW_SCREENCHANGE_SCREEN_REMOVED, &container_of(it, struct dpaw_screen_info_list_entry, screen_entry)->info);
    free(listener);
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
