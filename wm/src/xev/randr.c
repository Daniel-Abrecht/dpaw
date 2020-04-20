#include <dpaw/dpaw.h>
#include <dpaw/xev/X.c>
#include <dpaw/xev/randr.c>
#include <dpaw/dpawindow.h>
#include <stdio.h>

struct xev_event_specializer dpaw_xev_spec_RRNotify = {
  .type_offset = offsetof(XRRNotifyEvent, subtype),
  .type_size = sizeof(((XRRNotifyEvent*)0)->subtype),
};

int dpaw_xev_randr_init(struct dpaw* dpaw, struct xev_event_extension* extension){
  int event_base, error_base;
  int major=1, minor=2;
  (void)extension;
  (void)dpaw;
  if( !XRRQueryExtension(dpaw->root.display, &event_base, &error_base)
   || !XRRQueryVersion(dpaw->root.display, &major, &minor)
  ){
    fprintf(stderr, "X RandR extension not available\n");
    return -1;
  }
  if(major < 1 || (major == 1 && minor < 2)){
    printf("Server does not support XRandR 1.2 or newer\n");
    return -1;
  }
  extension->event_list[0]->first_event = event_base;
  return 0;
}

void dpaw_xev_randr_preprocess_event(struct dpaw* dpaw, XEvent* event){
  XRRUpdateConfiguration(event);
  (void)dpaw;
}

int dpaw_xev_randr_cleanup(struct dpaw* dpaw, struct xev_event_extension* extension){
  (void)extension;
  (void)dpaw;
  return 0;
}

enum event_handler_result dpaw_xev_randr_dispatch(struct dpaw* dpaw, struct xev_event* event){
  (void)dpaw;
  (void)event;
  return EHR_UNHANDLED;
}

int dpaw_xev_randr_subscribe(struct xev_event_extension* extension, struct dpawindow* window){
  if(window != &window->dpaw->root.window)
    return 0;
  if(!window->type->event_lookup_table.event_handler_list)
    return 0;
  unsigned long mask = 0;
  for(size_t j=0; j<dpaw_handler_list_count; j++){
    const struct dpaw_event_handler_list* list = &window->type->event_lookup_table.event_handler_list[j];
    if(!list->handler || !list->event_list)
      continue;
    for(size_t k=0,n=list->event_list->index_size; k<n; k++){
      const struct dpaw_event_handler* handler = &list->handler[k];
      if(!handler->callback || !handler->info)
        continue;
      for(const struct xev_event_info* evi=handler->info; evi && evi != &dpaw_xev_ev2ext_XEV_BaseEvent; evi=evi->event_list->parent_event){
        if(evi->type < 0)
          continue;
        if(evi->event_list->extension != extension)
          continue;
        mask |= evi->mask;
      }
    }
  }
  XRRSelectInput(window->dpaw->root.display, window->dpaw->root.window.xwindow, mask);
  return 0;
}

int dpaw_xev_randr_unsubscribe(struct xev_event_extension* extension, struct dpawindow* window){
  (void)extension;
  if(window != &window->dpaw->root.window)
    return 0;
  XRRSelectInput(window->dpaw->root.display, window->dpaw->root.window.xwindow, 0);
  return -1;
}
