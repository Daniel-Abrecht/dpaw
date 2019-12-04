#ifndef DPAWINDOW_APP_H
#define DPAWINDOW_APP_H

#include <dpawindow.h>
#include <workspace.h>
#include <linked_list.h>
#include <X11/Xutil.h>

/* There are some properties that need to be watched for changes, let's add an abstraction for that */
#define DPAW_APP_OBSERVABLE(T) \
  struct { \
    T value; \
    void* private; \
    int (*onchange)(void*, struct dpawindow_app*, T); \
  }

#define DPAW_APP_OBSERVABLE_NOTIFY(A,N) \
  do { \
    struct dpawindow_app* app_2 = (A); \
    if(app_2->observable.N.onchange) \
      app_2->observable.N.onchange(app_2->observable.N.private,app_2,app_2->observable.N.value); \
  } while(0)

#define DPAW_APP_OBSERVE(A,N,P,C) \
  do { \
    struct dpawindow_app* app_1 = (A); \
    app_1->observable.N.private = (P); \
    app_1->observable.N.onchange = (C); \
    DPAW_APP_OBSERVABLE_NOTIFY(app_1, N); \
  } while(0)

#define DPAW_APP_OBSERVABLE_SET(A,N,V) \
  do { \
    struct dpawindow_app* app_1 = (A); \
    app_1->observable.N.value = (V); \
    DPAW_APP_OBSERVABLE_NOTIFY(app_1, N); \
  } while(0)
/****/

#define DPAW_APP_STATE_LIST \
  X(_NET_WM_STATE_ABOVE) \
  X(_NET_WM_STATE_ADD) \
  X(_NET_WM_STATE_BELOW) \
  X(_NET_WM_STATE_DEMANDS_ATTENTION) \
  X(_NET_WM_STATE_FOCUSED) \
  X(_NET_WM_STATE_FULLSCREEN) \
  X(_NET_WM_STATE_HIDDEN) \
  X(_NET_WM_STATE_MAXIMIZED_HORZ) \
  X(_NET_WM_STATE_MAXIMIZED_VERT) \
  X(_NET_WM_STATE_MODAL) \
  X(_NET_WM_STATE_REMOVE) \
  X(_NET_WM_STATE_SHADED) \
  X(_NET_WM_STATE_SKIP_PAGER) \
  X(_NET_WM_STATE_SKIP_TASKBAR) \
  X(_NET_WM_STATE_STICKY) \
  X(_NET_WM_STATE_TOGGLE)

#define X(Y) bool Y : 1;
DECLARE_DPAW_DERIVED_WINDOW( app,
  struct dpaw_list_entry workspace_window_entry;
  struct dpaw_workspace* workspace;

  struct {
    DPAW_APP_STATE_LIST
  } wm_state;

  void* workspace_private;
  struct {
    DPAW_APP_OBSERVABLE(Atom) type;
    DPAW_APP_OBSERVABLE(XWMHints) window_hints;
    DPAW_APP_OBSERVABLE(XSizeHints) desired_placement;
  } observable;
  bool is_keyboard;
)
#undef X

int dpawindow_app_init(struct dpaw*, struct dpawindow_app*, Window);
int dpawindow_app_cleanup(struct dpawindow_app*);
int dpawindow_app_update_wm_state(struct dpawindow_app*);

#endif
