#include <dpawindow/root.h>
#include <screenchange.h>
#include <workspace.h>
#include <dpawin.h>
#include <stdio.h>

DEFINE_DPAWIN_DERIVED_WINDOW(root)

int dpawindow_root_init(struct dpawin* dpawin, struct dpawindow_root* window){
  if(dpawindow_root_init_super(dpawin, window) != 0){
    fprintf(stderr, "dpawindow_root_init_super failed\n");
    return -1;
  }
  if(dpawin_screenchange_init(&window->screenchange_detector, dpawin->root.display)){
    fprintf(stderr, "dpawin_screenchange_init failed\n");
    return -1;
  }
  if(dpawin_workspace_manager_init(dpawin, &window->workspace_manager) == -1){
    fprintf(stderr, "dpawin_workspace_init failed\n");
    return -1;
  }
  return 0;
}

int dpawindow_root_cleanup(struct dpawindow_root* window){
  dpawin_workspace_manager_destroy(&window->workspace_manager);
  return 0;
}

EV_ON(root, CreateNotify){
  printf("CreateNotify %lu\n", event->window);
  if(dpawin_workspace_manager_manage_window(&window->workspace_manager, event->window) != 0)
    return EHR_ERROR;
  return EHR_OK;
}

EV_ON(root, ReparentNotify){
  (void)window;
  printf("ReparentNotify %lu\n", event->window);
  return EHR_OK;
}
