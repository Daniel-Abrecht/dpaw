#include <xev/X.c>
#include <dpawindow/root.h>
#include <screenchange.h>
#include <workspace.h>
#include <dpawin.h>
#include <stdio.h>
#include <stdlib.h>

DEFINE_DPAWIN_DERIVED_WINDOW(root)

int dpawindow_root_init(struct dpawin* dpawin, struct dpawindow_root* window){
  dpawin->root.xev_list = calloc(dpawin_event_extension_count, sizeof(*dpawin->root.xev_list));
  if(!dpawin->root.xev_list){
    perror("calloc failed");
    goto error;
  }
  if(dpawindow_root_init_super(dpawin, window) != 0){
    fprintf(stderr, "dpawindow_root_init_super failed\n");
    goto error;
  }
  for(const struct xev_event_extension* extension=dpawin_event_extension_list; extension; extension=extension->next){
    struct dpawin_xev* xev = &dpawin->root.xev_list[extension->extension_index];
    xev->extension = -2; // Sentinel to check if initialisation worked properly. -1 is alredy used for the X fake extension
    if(extension->init(dpawin, xev)){
      fprintf(stderr, "Failed to initialise event extension %s\n", extension->name);
      if(extension->required)
        goto error;
      continue;
    }
    xev->xev = extension;
  }
  if(dpawin_screenchange_init(&window->screenchange_detector, window->display)){
    fprintf(stderr, "dpawin_screenchange_init failed\n");
    goto error;
  }
  if(dpawin_workspace_manager_init(&window->workspace_manager, dpawin) == -1){
    fprintf(stderr, "dpawin_workspace_init failed\n");
    goto error;
  }
  return 0;
error:
  if(dpawin->root.xev_list)
    free(dpawin->root.xev_list);
  return -1;
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

