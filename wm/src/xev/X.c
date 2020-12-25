#include <-dpaw/dpaw.h>
#include <-dpaw/xev/X.c>
#include <-dpaw/dpawindow.h>
#include <-dpaw/dpawindow/app.h>
#include <stddef.h>
#include <stdio.h>

int dpaw_xev_X_init(struct dpaw* dpaw, struct xev_event_extension* extension){
  (void)extension;
  (void)dpaw;
  return 0;
}

int dpaw_xev_X_cleanup(struct dpaw* dpaw, struct xev_event_extension* extension){
  (void)extension;
  (void)dpaw;
  return 0;
}

void dpaw_xev_X_preprocess_event(struct dpaw* dpaw, XEvent* event){
  (void)dpaw;
  (void)event;
}

enum event_handler_result dpaw_xev_X_dispatch(struct dpaw* dpaw, struct xev_event* event){
  if(event->info->type == GenericEvent || event->info->type == KeyPress || event->info->type == KeyRelease)
    return EHR_UNHANDLED;
  XEvent* ev = event->data;
  struct dpawindow* window = dpawindow_lookup(dpaw, ev->xany.window);
  struct dpawindow* target = 0;
  switch(event->info->type){
    case ConfigureRequest: {
      target = dpawindow_lookup(dpaw, (&ev->xany.window)[1]);
    } break;
    case ClientMessage: {
      if(!window)
        break;
      if(window->type == &dpawindow_type_app){
        struct dpawindow_app* app = container_of(window, struct dpawindow_app, window);
        if(app->workspace)
          target = app->workspace->window;
      }
    } break;
  }
  if(target == window)
    target = 0;
  enum event_handler_result result = EHR_UNHANDLED;
  if((result == EHR_UNHANDLED || result == EHR_NEXT)
   && target && target != &dpaw->root.window
  ) result = dpawindow_dispatch_event(target, event);
  if((result == EHR_UNHANDLED || result == EHR_NEXT)
   && window && window != &dpaw->root.window
  ) result = dpawindow_dispatch_event(window, event);
  return result;
}

int dpaw_xev_X_subscribe(struct xev_event_extension* extension, struct dpawindow* window){
  (void)extension;
  unsigned long mask = 0;
  struct dpawindow* set[] = {
    window,
    &window->dpaw->root.window
  };
  for(size_t i=0; i<sizeof(set)/sizeof(*set); i++){
    struct dpawindow* it = set[i];
    if(!it->type->event_lookup_table.event_handler_list)
      continue;
    for(size_t j=0; j<dpaw_handler_list_count; j++){
      const struct dpaw_event_handler_list* list = &it->type->event_lookup_table.event_handler_list[j];
      if(!list->handler || !list->event_list)
        continue;
      for(size_t k=0,n=list->event_list->index_size; k<n; k++){
        const struct dpaw_event_handler* handler = &list->handler[k];
        if(!handler->callback || !handler->info)
          continue;
        for(const struct xev_event_info* evi=handler->info; evi && evi != &dpaw_xev_ev2ext_XEV_BaseEvent; evi=evi->event_list->parent_event){
          if(evi->type < 0)
            continue;
          if(evi->event_list->parent_event != &dpaw_xev_ev2ext_XEV_BaseEvent)
            continue;
          mask |= evi->mask;
        }
      }
    }
  }
  // Flush previouse errors
  dpawindow_has_error_occured(window->dpaw->root.display);
  XSelectInput(window->dpaw->root.display, window->xwindow, mask);
  return dpawindow_has_error_occured(window->dpaw->root.display);
}

int dpaw_xev_X_unsubscribe(struct xev_event_extension* extension, struct dpawindow* window){
  (void)extension;
  XSelectInput(window->dpaw->root.display, window->xwindow, 0);
  return 0;
}

static void load_generic_event(struct dpaw* dpaw, void** data){
  XEvent* event = *data;
  if(XGetEventData(dpaw->root.display, &event->xcookie))
    *data = event->xcookie.data;
}

static void free_generic_event(struct dpaw* dpaw, void* old_data, void* new_data){
  (void)new_data;
  XEvent* event = old_data;
  XFreeEventData(dpaw->root.display, &event->xcookie);
}

struct xev_event_specializer dpaw_xev_spec_XEV_BaseEvent = {
  .type_offset = offsetof(XAnyEvent, type),
  .type_size = sizeof(((XAnyEvent*)0)->type),
};

struct xev_event_specializer dpaw_xev_spec_GenericEvent = {
  .type_offset = offsetof(XGenericEvent, evtype),
  .type_size = sizeof(((XGenericEvent*)0)->evtype),
  .extension_offset = offsetof(XGenericEvent, extension),
  .extension_size = sizeof(((XGenericEvent*)0)->extension),
  .load_data = load_generic_event,
  .free_data = free_generic_event,
};

