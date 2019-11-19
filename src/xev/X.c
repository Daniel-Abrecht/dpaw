#include <xev/X.c>
#include <stddef.h>
#include <dpawin.h>
#include <stdio.h>
#include <dpawindow.h>

int dpawin_xev_X_init(struct dpawin* dpawin, struct xev_event_extension* extension){
  (void)extension;
  (void)dpawin;
  return 0;
}

int dpawin_xev_X_cleanup(struct dpawin* dpawin, struct xev_event_extension* extension){
  (void)extension;
  (void)dpawin;
  return 0;
}

enum event_handler_result dpawin_xev_X_dispatch(struct dpawin* dpawin, struct xev_event* event){
  if(event->info->type == GenericEvent || event->info->type == KeyPress || event->info->type == KeyRelease)
    return EHR_UNHANDLED;
  XEvent* ev = event->data;
  struct dpawindow* window = 0;
  for(struct dpawin_list_entry* it=dpawin->window_list.first; it; it=it->next){
    struct dpawindow* wit = container_of(it, struct dpawindow, dpawin_window_entry);
    if(ev->xany.window != wit->xwindow)
      continue;
    window = wit;
    break;
  }
  if(!window || window->dpawin_window_entry.next == dpawin->window_list.first)
    return EHR_UNHANDLED;
  return dpawindow_dispatch_event(window, event);
}

int dpawin_xev_X_listen(struct xev_event_extension* extension, struct dpawindow* window){
  (void)extension;
  unsigned long mask = 0;
  struct dpawindow* set[] = {
    window,
    &window->dpawin->root.window
  };
  for(size_t i=0; i<sizeof(set)/sizeof(*set); i++){
    struct dpawindow* it = set[i];
    if(!it->type->event_lookup_table.event_handler_list)
      continue;
    for(size_t j=0; j<dpawin_handler_list_count; j++){
      const struct dpawin_event_handler_list* list = &it->type->event_lookup_table.event_handler_list[j];
      if(!list->handler || !list->event_list)
        continue;
      for(size_t k=0,n=list->event_list->index_size; k<n; k++){
        const struct dpawin_event_handler* handler = &list->handler[k];
        if(!handler->callback || !handler->info)
          continue;
        for(const struct xev_event_info* evi=handler->info; evi && evi != &dpawin_xev_ev2ext_XEV_BaseEvent; evi=evi->event_list->parent_event){
          if(evi->type < 0)
            continue;
          if(evi->event_list->parent_event != &dpawin_xev_ev2ext_XEV_BaseEvent)
            continue;
          mask |= 1ul << evi->type;
        }
      }
    }
  }
  // Flush previouse errors
  dpawindow_has_error_occured(window->dpawin->root.display);
  XSelectInput(window->dpawin->root.display, window->xwindow, mask);
  return dpawindow_has_error_occured(window->dpawin->root.display);
}

static void load_generic_event(struct dpawin* dpawin, void** data){
  XEvent* event = *data;
  if(XGetEventData(dpawin->root.display, &event->xcookie))
    *data = event->xcookie.data;
}

static void free_generic_event(struct dpawin* dpawin, void* data){
  XEvent* event = data;
  XFreeEventData(dpawin->root.display, &event->xcookie);
}

struct xev_event_specializer dpawin_xev_spec_XEV_BaseEvent = {
  .type_offset = offsetof(XAnyEvent, type),
  .type_size = sizeof(((XAnyEvent*)0)->type),
};

struct xev_event_specializer dpawin_xev_spec_GenericEvent = {
  .type_offset = offsetof(XGenericEvent, evtype),
  .type_size = sizeof(((XGenericEvent*)0)->evtype),
  .extension_offset = offsetof(XGenericEvent, extension),
  .extension_size = sizeof(((XGenericEvent*)0)->extension),
  .load_data = load_generic_event,
  .free_data = free_generic_event,
};

