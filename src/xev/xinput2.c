#include <dpawin.h>
#include <xev/X.c>
#include <xev/xinput2.c>
#include <X11/extensions/XInput2.h>
#include <stdio.h>
#include <stdlib.h>

int dpawin_xev_xinput2_init(struct dpawin* dpawin, struct xev_event_extension* extension){
  (void)extension;

  int opcode = 0;
  int first_event = 0;
  int first_error = 0;

  if(!XQueryExtension(dpawin->root.display, "XInputExtension", &opcode, &first_event, &first_error)) {
    printf("X Input extension not available.\n");
    return -1;
  }

  int major = 2, minor = 2;
  XIQueryVersion(dpawin->root.display, &major, &minor);
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
    dpawin->root.display,
    XIAllMasterDevices,
    dpawin->root.window.xwindow,
    false,
    &evmask,
    sizeof(modifiers)/sizeof(*modifiers), modifiers
  );

  extension->opcode = opcode;
  extension->first_error = first_error;

  return 0;
}

int dpawin_xev_xinput2_cleanup(struct dpawin* dpawin, struct xev_event_extension* extension){
  (void)dpawin;
  (void)extension;
  return 0;
}

void dpawin_xev_xinput2_preprocess_event(struct dpawin* dpawin, XEvent* event){
  (void)dpawin;
  (void)event;
}

enum event_handler_result dpawin_xev_xinput2_dispatch(struct dpawin* dpawin, struct xev_event* event){
  {
    // Let's ignore any fake events
    XAnyEvent* xany = event->data;
    if(xany->send_event)
      return EHR_UNHANDLED;
  }
  switch(event->info->type){
    case XI_TouchBegin:
    case XI_TouchUpdate:
    case XI_TouchEnd: {
      XIDeviceEvent* ev = event->data;
      // We've a grab on the root window for these events, so I'll have to figure out manually where to put them, because event=root, ...
      if(ev->event == dpawin->root.window.xwindow){
        struct dpawin_touchevent_window_map* twm = 0;
        for(int i=dpawin->last_touch; i-->0;){
          if(!dpawin->touch_source[i].window)
            continue;
          if(dpawin->touch_source[i].touchid != ev->detail)
            continue;
          twm = &dpawin->touch_source[i];
          break;
        }
        if(!twm && event->info->type == XI_TouchBegin){
          for(int i=0, n=dpawin->last_touch+1; i<DPAWIN_WORKSPACE_MAX_TOUCH_SOURCES && i<n; i++){
            if(dpawin->touch_source[i].window)
              continue;
            twm = &dpawin->touch_source[i];
            if(dpawin->last_touch <= i)
              dpawin->last_touch = i+1;
            break;
          }
          if(twm){
            twm->touchid = ev->detail;
            for(struct dpawin_workspace* workspace = dpawin->root.workspace_manager.workspace; workspace; workspace=workspace->next){
/*              printf("%lf>=%ld %lf>=%ld %lf<%ld %lf<%ld\n",
                ev->event_x, workspace->window->boundary.top_left.x,
                ev->event_y, workspace->window->boundary.top_left.y,
                ev->event_x, workspace->window->boundary.bottom_right.x,
                ev->event_y, workspace->window->boundary.bottom_right.y
              );*/
              if( ev->event_x <  workspace->window->boundary.top_left.x
               || ev->event_y <  workspace->window->boundary.top_left.y
               || ev->event_x >= workspace->window->boundary.bottom_right.x
               || ev->event_y >= workspace->window->boundary.bottom_right.y
              ) continue;
              twm->window = workspace->window;
              break;
            }
          }
        }
        if(twm && twm->window){
          ev->event = twm->window->xwindow;
          ev->event_x -= twm->window->boundary.top_left.x;
          ev->event_y -= twm->window->boundary.top_left.y;
          enum event_handler_result result = dpawindow_dispatch_event(twm->window, event);
          // Make sure to always accept or reject events at some point
          // TODO: Also reject them if it hasn't been accepted in a certain amount of time
          if(event->info->type == XI_TouchEnd && result == EHR_NEXT)
            result = EHR_UNHANDLED;
          if(result != EHR_OK && result != EHR_NEXT){
            twm->window = 0;
            XIAllowTouchEvents(
              dpawin->root.display,
              ev->deviceid,
              ev->detail,
              dpawin->root.window.xwindow,
              XIRejectTouch
            );
          }else{
            if(result == EHR_OK){
              XIAllowTouchEvents(
                dpawin->root.display,
                ev->deviceid,
                ev->detail,
                dpawin->root.window.xwindow,
                XIAcceptTouch
              );
            }
            if(event->info->type == XI_TouchEnd)
              twm->window = 0;
          }
          if(!twm->window && dpawin->last_touch == twm-dpawin->touch_source+1)
            while(dpawin->last_touch && dpawin->touch_source[dpawin->last_touch-1].window)
              dpawin->last_touch -= 1;
          return result;
        }else{
          // Just in case
          XIAllowTouchEvents(
            dpawin->root.display,
            ev->deviceid,
            ev->detail,
            dpawin->root.window.xwindow,
            XIRejectTouch
          );
          return EHR_UNHANDLED;
        }
      }
    } // fallthrough
    case XI_KeyPress:
    case XI_KeyRelease:
    case XI_ButtonPress:
    case XI_ButtonRelease:
    case XI_Motion: {
      XIDeviceEvent* ev = event->data;
      struct dpawindow* window = 0;
      for(struct dpawin_list_entry* it=dpawin->window_list.first; it; it=it->next){
        struct dpawindow* wit = container_of(it, struct dpawindow, dpawin_window_entry);
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

int dpawin_xev_xinput2_listen(struct xev_event_extension* extension, struct dpawindow* window){
  (void)extension;
  XIEventMask mask = {
    .deviceid = XIAllMasterDevices,
  };

  struct dpawindow* set[] = {window};

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
          if(evi->event_list->extension != &dpawin_xev_ext_xinput2)
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

  dpawindow_has_error_occured(window->dpawin->root.display);
  XISelectEvents(window->dpawin->root.display, window->xwindow, &mask, 1);

  if(mask.mask)
    free(mask.mask);

  return dpawindow_has_error_occured(window->dpawin->root.display);
}

