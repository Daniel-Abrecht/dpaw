#include <xev/X.c>
#include <xev/randr.c>
#include <dpawin.h>
#include <dpawindow.h>
#include <stdio.h>

struct xev_event_specializer dpawin_xev_spec_RRNotify = {
  .type_offset = offsetof(XRRNotifyEvent, subtype),
  .type_size = sizeof(((XRRNotifyEvent*)0)->subtype),
};

int dpawin_xev_randr_init(struct dpawin* dpawin, struct xev_event_extension* extension){
  int event_base, error_base;
  int major=1, minor=2;
  (void)extension;
  (void)dpawin;
  if( !XRRQueryExtension(dpawin->root.display, &event_base, &error_base)
   || !XRRQueryVersion(dpawin->root.display, &major, &minor)
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

void dpawin_xev_randr_preprocess_event(struct dpawin* dpawin, XEvent* event){
  XRRUpdateConfiguration(event);
  (void)dpawin;
}

int dpawin_xev_randr_cleanup(struct dpawin* dpawin, struct xev_event_extension* extension){
  (void)extension;
  (void)dpawin;
  return 0;
}

enum event_handler_result dpawin_xev_randr_dispatch(struct dpawin* dpawin, struct xev_event* event){
  (void)dpawin;
  (void)event;
  return EHR_UNHANDLED;
}

int dpawin_xev_randr_listen(struct xev_event_extension* extension, struct dpawindow* window){
  if(window != &window->dpawin->root.window)
    return -1;
  if(!window->type->event_lookup_table.event_handler_list)
    return 0;
  unsigned long mask = 0;
  for(size_t j=0; j<dpawin_handler_list_count; j++){
    const struct dpawin_event_handler_list* list = &window->type->event_lookup_table.event_handler_list[j];
    if(!list->handler || !list->event_list)
      continue;
    for(size_t k=0,n=list->event_list->index_size; k<n; k++){
      const struct dpawin_event_handler* handler = &list->handler[k];
      if(!handler->callback || !handler->info)
        continue;
      for(const struct xev_event_info* evi=handler->info; evi && evi != &dpawin_xev_ev2ext_XEV_BaseEvent; evi=evi->event_list->parent_event){
        if(evi->type < 0)
          continue;
        if(evi->event_list->extension != extension)
          continue;
        mask |= 1ul << evi->type;
      }
    }
  }
  XRRSelectInput(window->dpawin->root.display, window->dpawin->root.window.xwindow, mask);
  return 0;
}
