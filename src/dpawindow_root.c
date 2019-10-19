#include <dpawindow_root.h>
#include <screenchange.h>
#include <stdio.h>

DEFINE_DPAWIN_DERIVED_WINDOW(root)

static void screenchange_handler(void* ptr, enum dpawin_screenchange_type what, const struct dpawin_screen_info* info){
  struct dpawindow_root* window = ptr;
  (void)window;
  (void)what;
  printf("Screen %p size: %ux%u offset: %ux%u\n", (void*)info, info->boundary.size.x, info->boundary.size.y, info->boundary.position.x, info->boundary.position.y);
}

int dpawindow_root_init(struct dpawindow_root* window){
  if(dpawindow_root_init_super(window) != 0){
    fprintf(stderr, "dpawindow_root_init_super failed\n");
    return -1;
  }
  if(dpawin_screenchange_check() == -1){
    fprintf(stderr, "dpawin_screenchange_check failed\n");
    return -1;
  }
  if(dpawin_screenchange_listener_register(screenchange_handler, window) == -1){
    fprintf(stderr, "dpawin_screenchange_listener_register failed\n");
    return -1;
  }
  return 0;
}

int dpawindow_root_cleanup(struct dpawindow_root* window){
  dpawin_screenchange_listener_unregister(screenchange_handler, window);
  return 0;
}

EV_ON(root, MapRequest){
  XMapWindow(window->display, event->window);
  return EHR_OK;
}

EV_ON(root, ConfigureRequest){
  puts("ConfigureRequest");
  XWindowChanges changes = {
    .x = event->x,
    .y = event->y,
    .width  = event->width,
    .height = event->height,
    .border_width = event->border_width,
    .sibling      = event->above,
    .stack_mode   = event->detail
  };
  XConfigureWindow(window->display, event->window, event->value_mask, &changes);
  return EHR_OK;
}
