#include <workspace/handheld.h>
#include <stdio.h>

int init(struct dpawindow_workspace_handheld* workspace){
  (void)workspace;
  return 0;
}

void cleanup(struct dpawindow_workspace_handheld* workspace){
  (void)workspace;
}

int screen_added(struct dpawindow_workspace_handheld* workspace, struct dpawin_workspace_screen* screen){
  (void)workspace;
  (void)screen;
  return 0;
}

int screen_changed(struct dpawindow_workspace_handheld* workspace, struct dpawin_workspace_screen* screen){
  (void)workspace;
  (void)screen;
  return 0;
}

void screen_removed(struct dpawindow_workspace_handheld* workspace, struct dpawin_workspace_screen* screen){
  (void)workspace;
  (void)screen;
}


DEFINE_DPAWIN_WORKSPACE( handheld,
  .init = init,
  .cleanup = cleanup,
  .screen_added = screen_added,
  .screen_changed = screen_changed,
  .screen_removed = screen_removed
)
