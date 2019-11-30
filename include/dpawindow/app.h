#ifndef DPAWINDOW_APP_H
#define DPAWINDOW_APP_H

#include <dpawindow.h>
#include <workspace.h>
#include <linked_list.h>

/* There are some properties that need to be watched for changes, let's add an abstraction for that */
#define DPAW_APP_OBSERVABLE(T) \
  struct dpaw_observer { \
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
    if(app_1->observable.N.value == (V)) \
      break; \
    app_1->observable.N.value = (V); \
    DPAW_APP_OBSERVABLE_NOTIFY(app_1, N); \
  } while(0)
/****/

DECLARE_DPAW_DERIVED_WINDOW( app,
  struct dpaw_list_entry workspace_window_entry;
  struct dpaw_workspace* workspace;
  void* workspace_private;
  struct {
    DPAW_APP_OBSERVABLE(Atom) _NET_WM_WINDOW_TYPE;
  } observable;
)

int dpawindow_app_init(struct dpaw*, struct dpawindow_app*, Window);
int dpawindow_app_cleanup(struct dpawindow_app*);

#endif
