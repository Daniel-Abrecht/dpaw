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
  return 0;
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
  (void)extension;
  (void)window;
  return 0;
}
