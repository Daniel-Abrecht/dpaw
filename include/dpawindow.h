#ifndef DPAWINDOW_H
#define DPAWINDOW_H

#include <xev.h>
#include <primitives.h>
#include <linked_list.h>
#include <X11/Xlib.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

struct dpawindow;

struct dpawindow_type {
  const char* name;
  bool is_workspace;
  struct xev_event_lookup_table* extension_lookup_table_list;
};

struct dpawindow {
  const struct dpawindow_type* type;
  struct dpawin* dpawin;
  struct dpawin_list_entry dpawin_window_entry;
  Window xwindow;
  struct dpawin_rect boundary;
  bool mapped : 1;
  bool hidden : 1;
};

#define EV_ON(TYPE, EVENT) \
  enum event_handler_result dpawin_ev_on__ ## TYPE ## __ ## EVENT (struct dpawindow_  ## TYPE* window, xev_ ## EVENT ## _t* event); \
  __attribute__((used,constructor(1001))) \
  void dpawin_ev_init__ ## TYPE ## __ ## EVENT (void) { \
    extern struct dpawindow_type dpawindow_type_ ## TYPE; \
    if(dpawin_xev_add_event_handler( \
      &dpawindow_type_ ## TYPE.extension_lookup_table_list, \
      dpawin_xev_ev2ext_ ## EVENT, \
      EVENT, \
      (dpawin_event_handler_t)dpawin_ev_on__ ## TYPE ## __ ## EVENT \
    )) _Exit(1); \
  } \
  enum event_handler_result dpawin_ev_on__ ## TYPE ## __ ## EVENT (struct dpawindow_  ## TYPE* window, xev_ ## EVENT ## _t* event)

#define DECLARE_DPAWIN_DERIVED_WINDOW(NAME, ...) \
  struct dpawindow_ ## NAME { \
    struct dpawindow window; /* Must be the first member */ \
    __VA_ARGS__ \
  }; \
  extern struct dpawindow_type dpawindow_type_ ## TYPE; \
  int dpawindow_ ## NAME ## _init_super(struct dpawin*, struct dpawindow_ ## NAME*); \
  int dpawindow_ ## NAME ## _cleanup_super(struct dpawindow_ ## NAME*);

#define DEFINE_DPAWIN_DERIVED_WINDOW(NAME) \
  struct dpawindow_type dpawindow_type_ ## NAME = { \
    .name = #NAME \
  }; \
  __attribute__((constructor)) \
  static void dpawindow_type_constructor_ ## NAME(void){ \
    dpawindow_type_ ## NAME.is_workspace = !strncmp("workspace_", #NAME, 10); \
  } \
  int dpawindow_ ## NAME ## _init_super(struct dpawin* dpawin, struct dpawindow_ ## NAME* w){ \
    w->window.type = &dpawindow_type_ ## NAME; \
    w->window.dpawin = dpawin; \
    if(dpawindow_register(&w->window)) \
      return -1; \
    return 0; \
  } \
  int dpawindow_ ## NAME ## _cleanup_super(struct dpawindow_ ## NAME* w){ \
    if(dpawindow_unregister(&w->window)) \
      return -1; \
    return 0; \
  }

bool dpawindow_has_error_occured(Display* display);
struct dpawindow* dpawindow_lookup(struct dpawin*, Window);
enum event_handler_result dpawindow_dispatch_event(struct dpawindow* window, const struct xev_event_extension*, int event, void* data);
int dpawindow_hide(struct dpawindow* window, bool hidden);
int dpawindow_set_mapping(struct dpawindow* window, bool mapping);
int dpawindow_place_window(struct dpawindow*, struct dpawin_rect boundary);
int dpawindow_register(struct dpawindow* window);
int dpawindow_unregister(struct dpawindow* window);

#endif
