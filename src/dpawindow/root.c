#include <dpawindow/root.h>
#include <stdio.h>

DEFINE_DPAWIN_DERIVED_WINDOW(root)

int dpawindow_root_init(struct dpawindow_root* window){
  if(dpawindow_root_init_super(window) != 0){
    fprintf(stderr, "dpawindow_root_init_super failed\n");
    return -1;
  }
  if(dpawin_workspace_manager_init(&window->workspace_manager, window) == -1){
    fprintf(stderr, "dpawin_workspace_init failed\n");
    return -1;
  }
  return 0;
}

int dpawindow_root_cleanup(struct dpawindow_root* window){
  dpawin_workspace_manager_destroy(&window->workspace_manager);
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
