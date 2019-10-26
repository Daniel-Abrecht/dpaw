#include <dpawin.h>
#include <touch.h>
#include <X11/extensions/XInput2.h>
#include <stdio.h>
#include <stdlib.h>

int dpawin_touch_init(struct dpawin_touch_manager* tmgr, struct dpawin* dpawin){
  tmgr->dpawin = dpawin;

  int major_opcode = 0;
  int first_event = 0; // Note: the last one is XI2LASTEVENT, which should be the same as XI_LASTEVENT for backwards compat.
  int first_error = 0;

  if(!XQueryExtension(tmgr->dpawin->root.display, "XInputExtension", &major_opcode, &first_event, &first_error)) {
    printf("X Input extension not available.\n");
    return 1;
  }

  int major = 2, minor = 2;
  XIQueryVersion(tmgr->dpawin->root.display, &major, &minor);
  if(major < 2 || (major == 2 && minor < 2)){
    printf("Server does not support XI 2.2 or newer\n");
    return -1;
  }

  XIEventMask mask = {
    .deviceid = XIAllMasterDevices,
  };
#define EVENTS \
  X(XI_TouchBegin) \
  X(XI_TouchUpdate) \
  X(XI_TouchEnd)
#define X(Y) { \
    int len = XIMaskLen(Y); \
    if(mask.mask_len < len) \
      mask.mask_len = len; \
  }
  EVENTS
#undef X

  if(mask.mask_len <= 0){
    printf("Impossible mask_len\n");
    return -1;
  }

  mask.mask = calloc(mask.mask_len, sizeof(char));
  if(!mask.mask){
    fprintf(stderr, "calloc failed\n");
    return -1;
  }

#define X(Y) XISetMask(mask.mask, Y);
  EVENTS
#undef X
#undef EVENTS

  XISelectEvents(tmgr->dpawin->root.display, tmgr->dpawin->root.window.xwindow, &mask, 1);

  free(mask.mask);

  return 0;
}
