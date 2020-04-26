#include <-dpaw/dpaw.h>
#include <-dpaw/xev/X.c>
#include <-dpaw/xev/xinput2.c>
#include <X11/extensions/XInput2.h>
#include <stdio.h>
#include <stdlib.h>

int dpaw_xev_xinput2_init(struct dpaw* dpaw, struct xev_event_extension* extension){
  (void)extension;

  int opcode = 0;
  int first_event = 0;
  int first_error = 0;

  if(!XQueryExtension(dpaw->root.display, "XInputExtension", &opcode, &first_event, &first_error)) {
    printf("X Input extension not available.\n");
    return -1;
  }

  int major = 2, minor = 2;
  XIQueryVersion(dpaw->root.display, &major, &minor);
  if(major < 2 || (major == 2 && minor < 2)){
    printf("Server does not support XI 2.2 or newer\n");
    return -1;
  }

  unsigned char mask[XIMaskLen(XI_LASTEVENT)] = {0};
  XISetMask(mask, XI_TouchBegin);
  XISetMask(mask, XI_TouchUpdate);
  XISetMask(mask, XI_TouchEnd);
  XIEventMask evmask = {
    .mask_len = sizeof(mask),
    .mask = mask
  };

  XIGrabModifiers modifiers[] = {
    {
      .modifiers = XIAnyModifier
    }
  };
  XIGrabTouchBegin(
    dpaw->root.display,
    XIAllMasterDevices,
    dpaw->root.window.xwindow,
    false,
    &evmask,
    sizeof(modifiers)/sizeof(*modifiers), modifiers
  );

  extension->opcode = opcode;
  extension->first_error = first_error;

  return 0;
}

int dpaw_xev_xinput2_cleanup(struct dpaw* dpaw, struct xev_event_extension* extension){
  (void)dpaw;
  (void)extension;
  return 0;
}

void dpaw_xev_xinput2_preprocess_event(struct dpaw* dpaw, XEvent* event){
  (void)dpaw;
  (void)event;
}

enum event_handler_result dpaw_xev_xinput2_dispatch(struct dpaw* dpaw, struct xev_event* event){
  {
    // Let's unsubscribe any fake events
    XAnyEvent* xany = event->data;
    if(xany->send_event)
      return EHR_UNHANDLED;
  }
  switch(event->info->type){
    case XI_TouchBegin:
    case XI_TouchUpdate:
    case XI_TouchEnd: {
      struct dpaw_touch_event* te = event->data;
      if(te->event.detail == -1)
        return EHR_UNHANDLED;
      if(te->twm){
        enum event_handler_result result = dpawindow_dispatch_event(te->twm->window, event);
        // Make sure to always accept or reject events at some point
        // TODO: Also reject them if it hasn't been accepted in a certain amount of time
        if(event->info->type == XI_TouchEnd && result == EHR_NEXT)
          result = EHR_UNHANDLED;
        if(result != EHR_OK && result != EHR_NEXT){
          te->twm->window = 0;
        }else{
          if(result == EHR_OK){
            XIAllowTouchEvents(
              dpaw->root.display,
              te->event.deviceid,
              te->event.detail,
              dpaw->root.window.xwindow,
              XIAcceptTouch
            );
          }
          if(event->info->type == XI_TouchEnd)
            te->twm->window = 0;
        }
        return result;
      }
    } // fallthrough
    case XI_KeyPress:
    case XI_KeyRelease:
    case XI_ButtonPress:
    case XI_ButtonRelease:
    case XI_Motion: {
      XIDeviceEvent* ev = event->data;
      struct dpawindow* window = 0;
      for(struct dpaw_list_entry* it=dpaw->window_list.first; it; it=it->next){
        struct dpawindow* wit = container_of(it, struct dpawindow, dpaw_window_entry);
        if(ev->event != wit->xwindow)
          continue;
        window = wit;
        break;
      }
      if(!window)
        break;
      return dpawindow_dispatch_event(window, event);
    } break;
  }
  return EHR_UNHANDLED;
}

int dpaw_xev_xinput2_subscribe(struct xev_event_extension* extension, struct dpawindow* window){
  (void)extension;
  XIEventMask mask = {
    .deviceid = XIAllMasterDevices,
  };

  struct dpawindow* set[] = {window};

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
          if(evi->event_list->extension != extension)
            continue;
          int len = XIMaskLen(evi->type);
          if(mask.mask_len < len)
            mask.mask_len = len;
        }
      }
    }
  }

  if(mask.mask_len < 0){
    printf("Impossible mask_len\n");
    return -1;
  }

  if(mask.mask_len){
    mask.mask = calloc(mask.mask_len, sizeof(char));
    if(!mask.mask){
      fprintf(stderr, "calloc failed\n");
      return -1;
    }
  }

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
          if(evi->event_list->extension != &dpaw_xev_ext_xinput2)
            continue;
          int type = evi->type;
          if( window->type->is_workspace && (
              type == XI_TouchBegin
           || type == XI_TouchUpdate
           || type == XI_TouchEnd
          )) continue;
          XISetMask(mask.mask, type);
        }
      }
    }
  }

  dpawindow_has_error_occured(window->dpaw->root.display);
  XISelectEvents(window->dpaw->root.display, window->xwindow, &mask, 1);

  if(mask.mask)
    free(mask.mask);

  return dpawindow_has_error_occured(window->dpaw->root.display);
}

int dpaw_xev_xinput2_unsubscribe(struct xev_event_extension* extension, struct dpawindow* window){
  (void)extension;
  XIEventMask mask = {
    .deviceid = XIAllMasterDevices,
  };
  XISelectEvents(window->dpaw->root.display, window->xwindow, &mask, 1);
  return 0;
}

static void load_touch_event(struct dpaw* dpaw, void** data){
  struct dpaw_touch_event* te = calloc(sizeof(struct dpaw_touch_event), 1);
  assert(te);
  te->touch_source = -1;
  {
    XIDeviceEvent* ev = *data;
    *data = te;
    te->event = *ev;
  }

  if(te->event.event == dpaw->root.window.xwindow){
    for(int i=dpaw->last_touch; i-->0;){
      if(!dpaw->touch_source[i].window)
        continue;
      if(dpaw->touch_source[i].touchid != te->event.detail+1)
        continue;
      te->twm = &dpaw->touch_source[i];
      te->touch_source = i;
      break;
    }
    if(!te->twm && te->event.evtype == XI_TouchBegin){
      for(int i=0, n=dpaw->last_touch+1; i<DPAW_WORKSPACE_MAX_TOUCH_SOURCES && i<n; i++){
        if(dpaw->touch_source[i].window)
          continue;
        te->twm = &dpaw->touch_source[i];
        te->touch_source = i;
        if(dpaw->last_touch <= i)
          dpaw->last_touch = i+1;
        break;
      }
      if(te->twm){
        te->twm->touchid = te->event.detail+1;
        for(struct dpaw_list_entry* wlit = dpaw->root.workspace_manager.workspace_list.first; wlit; wlit=wlit->next){
          struct dpaw_workspace* workspace = container_of(wlit, struct dpaw_workspace, wmgr_workspace_list_entry);
          if( te->event.event_x <  workspace->window->boundary.top_left.x
            || te->event.event_y <  workspace->window->boundary.top_left.y
            || te->event.event_x >= workspace->window->boundary.bottom_right.x
            || te->event.event_y >= workspace->window->boundary.bottom_right.y
          ) continue;
          te->twm->window = workspace->window;
          break;
        }
      }
    }
    if(te->twm && te->twm->window){
      te->event.event    = te->twm->window->xwindow;
      te->event.event_x -= te->twm->window->boundary.top_left.x;
      te->event.event_y -= te->twm->window->boundary.top_left.y;
    }else{
      if(te->twm){
        if(dpaw->last_touch == te->twm-dpaw->touch_source+1)
          while(dpaw->last_touch && dpaw->touch_source[dpaw->last_touch-1].window)
            dpaw->last_touch -= 1;
        memset(te->twm, 0, sizeof(*te->twm));
      }
      te->twm = 0;
      te->touch_source = -1;
      XIAllowTouchEvents(
        dpaw->root.display,
        te->event.deviceid,
        te->event.detail,
        dpaw->root.window.xwindow,
        XIRejectTouch
      );
    }
  }
}

static void free_touch_event(struct dpaw* dpaw, void* old_data, void* new_data){
  (void)dpaw;
  (void)old_data;
  struct dpaw_touch_event* te = new_data;
  if(te->twm && !te->twm->window){
    XIAllowTouchEvents(
      dpaw->root.display,
      te->event.deviceid,
      te->event.detail,
      dpaw->root.window.xwindow,
      XIRejectTouch
    );
    if(dpaw->last_touch == te->twm-dpaw->touch_source+1)
      while(dpaw->last_touch && dpaw->touch_source[dpaw->last_touch-1].window)
        dpaw->last_touch -= 1;
    memset(te->twm, 0, sizeof(*te->twm));
    te->twm = 0;
    te->touch_source = -1;
  }
  free(te);
}

struct xev_event_specializer dpaw_xev_spec_XI_TouchBegin = {
  .load_data = load_touch_event,
  .free_data = free_touch_event,
};

struct xev_event_specializer dpaw_xev_spec_XI_TouchUpdate = {
  .load_data = load_touch_event,
  .free_data = free_touch_event,
};

struct xev_event_specializer dpaw_xev_spec_XI_TouchEnd = {
  .load_data = load_touch_event,
  .free_data = free_touch_event,
};

