#include <dpawin.h>
#include <X11/extensions/XInput2.h>
#include <stdio.h>
#include <stdlib.h>

int dpawin_xev_xinput2_init(struct dpawin* dpawin, struct dpawin_xev* xev){
  int major_opcode = 0;
  int first_event = 0;
  int first_error = 0;

  if(!XQueryExtension(dpawin->root.display, "XInputExtension", &major_opcode, &first_event, &first_error)) {
    printf("X Input extension not available.\n");
    return -1;
  }

  int major = 2, minor = 2;
  XIQueryVersion(dpawin->root.display, &major, &minor);
  if(major < 2 || (major == 2 && minor < 2)){
    printf("Server does not support XI 2.2 or newer\n");
    return -1;
  }

  xev->extension = major_opcode;

  return 0;
}

int dpawin_xev_xinput2_cleanup(struct dpawin* dpawin, struct dpawin_xev* xev){
  (void)dpawin;
  (void)xev;
  return 0;
}

enum event_handler_result dpawin_xev_xinput2_dispatch(struct dpawin* dpawin, struct dpawin_xev* xev, int event, void* data){
  switch(event){
    case XI_KeyPress:
    case XI_KeyRelease:
    case XI_ButtonPress:
    case XI_ButtonRelease:
    case XI_Motion:
    case XI_TouchBegin:
    case XI_TouchUpdate:
    case XI_TouchEnd: {
      XIDeviceEvent* ev = data;
      struct dpawindow* it = 0;
      for(it = &dpawin->root.window; it; it=it->next)
        if(ev->event == it->xwindow)
          break;
      if(!it)
        break;
      return dpawindow_dispatch_event(it, xev->xev, event, ev);
    } break;
  }
  return EHR_UNHANDLED;
}

int dpawin_xev_xinput2_listen(struct dpawin_xev* xev, struct dpawindow* window){
  XIEventMask mask = {
    .deviceid = XIAllMasterDevices,
  };

  struct dpawindow* set[] = {window};

  for(size_t i=0; i<sizeof(set)/sizeof(*set); i++){
    struct dpawindow* it = set[i];
    struct xev_event_lookup_table* table = it->type->extension_lookup_table_list;
    if(table && table->handler){
      table += xev->xev->extension_index;
      size_t size = xev->xev->info_size;
      for(size_t j=1; j<size; j++){
        if(!table->handler[j])
          continue;
        int len = XIMaskLen(xev->xev->info[j].type);
        if(mask.mask_len < len)
          mask.mask_len = len;
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
    struct xev_event_lookup_table* table = it->type->extension_lookup_table_list;
    if(table && table->handler){
      table += xev->xev->extension_index;
      size_t size = xev->xev->info_size;
      for(size_t j=1; j<size; j++){
        if(table->handler[j])
          XISetMask(mask.mask, xev->xev->info[j].type);
      }
    }
  }

  dpawindow_has_error_occured(window->dpawin->root.display);
  XISelectEvents(window->dpawin->root.display, window->xwindow, &mask, 1);
  if(mask.mask)
    free(mask.mask);

  return dpawindow_has_error_occured(window->dpawin->root.display);
}

