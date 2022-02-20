#include <-dpaw/dpaw.h>
#include <-dpaw/xev/X.c>
#include <-dpaw/xev/xinput2.c>
#include <X11/extensions/XInput2.h>
#include <stdio.h>
#include <stdlib.h>

static enum dpaw_input_device_type input_type_from_use(int use){
  if(use & XIMasterKeyboard)
    return DPAW_INPUT_DEVICE_TYPE_MASTER_KEYBOARD;
  if(use & XIMasterPointer)
    return DPAW_INPUT_DEVICE_TYPE_MASTER_POINTER;
  if(use & XISlaveKeyboard)
    return DPAW_INPUT_DEVICE_TYPE_KEYBOARD;
  if(use & XISlavePointer)
    return DPAW_INPUT_DEVICE_TYPE_POINTER;
  return -1;
}

static struct dpaw_input_device* allocate_input_device(struct dpaw* dpaw, enum dpaw_input_device_type type, int deviceid){
  struct dpaw_input_device* ret = 0;
  switch(type){
    default: return 0;
    case DPAW_INPUT_DEVICE_TYPE_MASTER_KEYBOARD: {
      struct dpaw_input_master_device* ddev = calloc(1, sizeof(struct dpaw_input_master_device));
      if(!ddev) return 0;
      dpaw_linked_list_set(&dpaw->input_device.master_list, &ddev->id_master_entry, 0);
      ddev->pointer.deviceid = -1;
      ret = &ddev->keyboard;
    } break;
    case DPAW_INPUT_DEVICE_TYPE_MASTER_POINTER: {
      struct dpaw_input_master_device* ddev = calloc(1, sizeof(struct dpaw_input_master_device));
      if(!ddev) return 0;
      dpaw_linked_list_set(&dpaw->input_device.master_list, &ddev->id_master_entry, 0);
      ddev->keyboard.deviceid = -1;
      ret = &ddev->pointer;
    } break;
    case DPAW_INPUT_DEVICE_TYPE_KEYBOARD: {
      struct dpaw_input_keyboard_device* ddev = calloc(1, sizeof(struct dpaw_input_keyboard_device));
      if(!ddev) return 0;
      dpaw_linked_list_set(&dpaw->input_device.keyboard_list, &ddev->id_keyboard_entry, 0);
      ret = &ddev->device;
    } break;
    case DPAW_INPUT_DEVICE_TYPE_POINTER: {
      struct dpaw_input_pointer_device* ddev = calloc(1, sizeof(struct dpaw_input_pointer_device));
      if(!ddev) return 0;
      dpaw_linked_list_set(&dpaw->input_device.pointer_list, &ddev->id_pointer_entry, 0);
      ret = &ddev->device;
    } break;
  }
  ret->type = type;
  ret->deviceid = deviceid;
  return ret;
}

static struct dpaw_input_device* find_input_device_by_id(struct dpaw* dpaw, int deviceid){
  for(struct dpaw_list_entry* it=dpaw->input_device.master_list.first; it; it=it->next){
    struct dpaw_input_master_device* mdev = container_of(it, struct dpaw_input_master_device, id_master_entry);
    if(mdev->keyboard.deviceid == deviceid)
      return &mdev->keyboard;
    if(mdev->pointer.deviceid == deviceid)
      return &mdev->pointer;
  }
  for(struct dpaw_list_entry* it=dpaw->input_device.keyboard_list.first; it; it=it->next){
    struct dpaw_input_keyboard_device* kdev = container_of(it, struct dpaw_input_keyboard_device, id_keyboard_entry);
    if(kdev->device.deviceid == deviceid)
      return &kdev->device;
  }
  for(struct dpaw_list_entry* it=dpaw->input_device.pointer_list.first; it; it=it->next){
    struct dpaw_input_pointer_device* pdev = container_of(it, struct dpaw_input_pointer_device, id_pointer_entry);
    if(pdev->device.deviceid == deviceid)
      return &pdev->device;
  }
  return 0;
}

static int remove_device(struct dpaw* dpaw, struct dpaw_input_device* device){
  (void)dpaw;
  (void)device;
  {
    struct dpaw_input_master_device* mdev = 0;
    device->deviceid = -1;
    if(device->type == DPAW_INPUT_DEVICE_TYPE_MASTER_KEYBOARD){
      mdev = container_of(device, struct dpaw_input_master_device, keyboard);
    }else if(device->type == DPAW_INPUT_DEVICE_TYPE_MASTER_POINTER){
      mdev = container_of(device, struct dpaw_input_master_device, pointer);
      dpaw_linked_list_set(0, &mdev->drag_event_owner, 0);
    }
    if(mdev){
      if(mdev->keyboard.deviceid == -1 && mdev->pointer.deviceid == -1){
        dpaw_linked_list_set(0, &mdev->id_master_entry, 0);
        dpaw_linked_list_set(0, &mdev->drag_event_owner, 0);
        free(mdev);
      }
      return 0;
    }
  }
  if(device->type == DPAW_INPUT_DEVICE_TYPE_KEYBOARD){
    struct dpaw_input_keyboard_device* kdev = container_of(device, struct dpaw_input_keyboard_device, device);
    dpaw_linked_list_set(0, &kdev->id_keyboard_entry, 0);
    free(kdev);
    return 0;
  }
  if(device->type == DPAW_INPUT_DEVICE_TYPE_POINTER){
    struct dpaw_input_pointer_device* pdev = container_of(device, struct dpaw_input_pointer_device, device);
    dpaw_linked_list_set(0, &pdev->id_pointer_entry, 0);
    free(pdev);
    return 0;
  }
  return -1;
}

static int remove_device_by_id(struct dpaw* dpaw, int deviceid){
  printf("Lost input device: %d\n", deviceid);
  struct dpaw_input_device* device = find_input_device_by_id(dpaw, deviceid);
  if(!device)
    return 0;
  return remove_device(dpaw, device);
}

static int add_or_update_device(struct dpaw* dpaw, int deviceid){
  int ndevices_return = 0;
  XIDeviceInfo* info = XIQueryDevice(
    dpaw->root.display,
    deviceid,
    &ndevices_return
  );
  if(!info)
    return -1;
  if(ndevices_return != 1)
    goto error;
  if(info->deviceid != deviceid)
    goto error;
  enum dpaw_input_device_type type = input_type_from_use(info->use);
  if(type == (enum dpaw_input_device_type)-1)
    goto error;
  struct dpaw_input_device* old = find_input_device_by_id(dpaw, deviceid);
  if(old){
    printf("Updating input device: %d %s\n", info->deviceid, info->name);
    if(old->type != type){
      printf("Device type changed\n");
      remove_device(dpaw, old);
      old = 0;
    }
  }else{
    printf("Got input device: %d %s\n", info->deviceid, info->name);
  }
  if(!old)
    old = allocate_input_device(dpaw, type, deviceid);
  if(!old)
    goto error;

  XIFreeDeviceInfo(info);
  return 0;
error:
  XIFreeDeviceInfo(info);
  return -1;
}

EV_ON(root, XI_HierarchyChanged){
  for(size_t i=0,n=event->num_info; i<n; i++){
    XIHierarchyInfo* info = &event->info[i];
    if(info->flags & (XIMasterRemoved|XISlaveRemoved)){
      remove_device_by_id(window->window.dpaw, info->deviceid);
    }else{
      add_or_update_device(window->window.dpaw, info->deviceid);
    }
  }
  return EHR_OK;
}

EV_ON(root, XI_DeviceChanged){
  printf("XI_DeviceChanged %u %u %u %u\n", event->deviceid, event->sourceid, event->reason, event->num_classes);
  return EHR_OK;
}

static inline XIEventMask* button_grab_mask(void){
  static unsigned char mask[XIMaskLen(XI_LASTEVENT)] = {0};
  XISetMask(mask, XI_ButtonPress);
  XISetMask(mask, XI_Motion);
  XISetMask(mask, XI_ButtonRelease);
  static XIEventMask evmask = {
    .mask_len = sizeof(mask),
    .mask = mask
  };
  return &evmask;
}

int dpaw_own_drag_event(struct dpaw* dpaw, const xev_XI_ButtonRelease_t* event, struct dpaw_input_drag_event_owner* owner){
  struct dpaw_input_device* device = find_input_device_by_id(dpaw, event->deviceid);
  if(!device){
    printf("dpaw_own_drag_event: device not found\n");
    return -1;
  }
  if(device->type != DPAW_INPUT_DEVICE_TYPE_MASTER_POINTER){
    printf("dpaw_own_drag_event: Wrong device type\n");
    return -1;
  }
  struct dpaw_input_master_device* mdev = container_of(device, struct dpaw_input_master_device, pointer);
  dpaw_linked_list_set(&owner->device_owner_list, &mdev->drag_event_owner, 0);
  XIGrabDevice(dpaw->root.display, event->deviceid, dpaw->root.window.xwindow, CurrentTime,  None, GrabModeAsync, GrabModeAsync, False, button_grab_mask());
  return 0;
}

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

  unsigned char mask[XIMaskLen(XI_LASTEVENT)];
  XIEventMask evmask = {
    .mask_len = sizeof(mask),
    .mask = mask
  };
  XIGrabModifiers modifiers[] = {
    {
      .modifiers = XIAnyModifier
    }
  };

  // XIGrabButton must be before XIGrabTouchBegin!!!
  // For unknown reason, we won't get any touch events otherwise anymore!!!

  memset(evmask.mask, 0, evmask.mask_len);
  XISetMask(mask, XI_ButtonPress);
  XIGrabButton(
    dpaw->root.display,
    XIAllMasterDevices,
    Button1,
    dpaw->root.window.xwindow,
    0,
    GrabModeSync,
    GrabModeAsync,
    false,
    &evmask,
    sizeof(modifiers)/sizeof(*modifiers), modifiers
  );

  memset(evmask.mask, 0, evmask.mask_len);
  XISetMask(mask, XI_TouchBegin);
  XISetMask(mask, XI_TouchUpdate);
  XISetMask(mask, XI_TouchEnd);

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

  int ndevices_return = 0;
  XIDeviceInfo* info = XIQueryDevice(
    dpaw->root.display,
    XIAllDevices,
    &ndevices_return
  );
  for(size_t i=0,n=ndevices_return; i<n; i++)
    add_or_update_device(dpaw, info->deviceid);
  XIFreeDeviceInfo(info);

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

//  printf("[[%s]]\n", event->info->name);
  switch(event->info->type){
    case XI_TouchBegin:
    case XI_TouchUpdate:
    case XI_TouchEnd: {
      struct dpaw_touch_event* te = event->data;
      if(te->event.detail == -1)
        return EHR_UNHANDLED;
      if(te->twm){
        if(XI_TouchBegin == event->info->type){
          struct dpaw_workspace* workspace = dpawindow_to_dpaw_workspace(te->twm->window);
          if(workspace)
            dpaw_workspace_set_active(0, workspace);
        }
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
      if(event->info->type == XI_Motion || event->info->type == XI_ButtonRelease){
        struct dpaw_input_device* device = find_input_device_by_id(dpaw, ev->deviceid);
        if(!device){
          printf("dpaw_xev_xinput2_dispatch: device not found\n");
        }else if(device->type != DPAW_INPUT_DEVICE_TYPE_MASTER_POINTER){
          printf("dpaw_xev_xinput2_dispatch: Wrong device type\n");
        }else{
          struct dpaw_input_master_device* mdev = container_of(device, struct dpaw_input_master_device, pointer);
          if(mdev->drag_event_owner.list){
            struct dpaw_input_drag_event_owner* deo = container_of(mdev->drag_event_owner.list, struct dpaw_input_drag_event_owner, device_owner_list);
            if(deo->handler){
              if(event->info->type == XI_Motion && deo->handler->onmove){
                deo->handler->onmove(deo, mdev, ev);
              }else{
                if(deo->handler->ondrop)
                  deo->handler->ondrop(deo, mdev, ev);
                XIUngrabDevice(dpaw->root.display, ev->deviceid, CurrentTime);
              }
            }
            return EHR_OK;
          }
          if(event->info->type == XI_ButtonRelease)
            dpaw_linked_list_set(0, &mdev->drag_event_owner, 0);
        }
      }
      if(event->info->type == XI_ButtonPress){
        for(struct dpaw_list_entry* wlit = dpaw->root.workspace_manager.workspace_list.first; wlit; wlit=wlit->next){
          struct dpaw_workspace* workspace = container_of(wlit, struct dpaw_workspace, wmgr_workspace_list_entry);
          if( ev->root_x <  workspace->window->boundary.top_left.x
            || ev->root_y <  workspace->window->boundary.top_left.y
            || ev->root_x >= workspace->window->boundary.bottom_right.x
            || ev->root_y >= workspace->window->boundary.bottom_right.y
          ) continue;
          dpaw_workspace_set_active(0, workspace);
          break;
        }
      }
      struct dpawindow* ewin = 0;
      struct dpawindow* cwin = 0;
      for(struct dpaw_list_entry* it=dpaw->window_list.first; it; it=it->next){
        struct dpawindow* wit = container_of(it, struct dpawindow, dpaw_window_entry);
        if(!cwin && ev->child == wit->xwindow)
          cwin = wit;
        if(!ewin && ev->event == wit->xwindow)
          ewin = wit;
        if((ewin||!ev->event) && (cwin||!ev->child))
          break;
      }
      enum event_handler_result result = EHR_UNHANDLED;
      if(ewin)
        result = dpawindow_dispatch_event(ewin, event);
      if(cwin && (result == EHR_UNHANDLED || result == EHR_NEXT))
        result = dpawindow_dispatch_event(cwin, event);
      if(event->info->type == XI_ButtonPress)
        XIAllowEvents(dpaw->root.display, ev->deviceid, XIReplayDevice, CurrentTime);
      if(event->info->type == XI_ButtonRelease)
        XIUngrabDevice(dpaw->root.display, ev->deviceid, CurrentTime);
//      printf("[[%s]] %lx %lx %lx %.1fx%.1f %.1fx%.1f\n", event->info->name, ev->event, ev->child, ev->root, ev->event_x, ev->event_y, ev->root_x, ev->root_y);
      return result;
    } break;
  }
  return EHR_UNHANDLED;
}

int dpaw_xev_xinput2_subscribe(struct xev_event_extension* extension, struct dpawindow* window){
  (void)extension;
  XIEventMask mask = {
    .deviceid = XIAllDevices,
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
          if( window->type->workspace_type && (
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
    .deviceid = XIAllDevices,
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

