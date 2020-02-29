#ifndef DPAWINDOW_H
#define DPAWINDOW_H

#include <dpaw/xev.h>
#include <dpaw/primitives.h>
#include <dpaw/linked_list.h>
#include <X11/Xlib.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#ifndef NO_UNUSED_ATTR
#define UNUSED_ATTR __attribute__((unused))
#endif

struct dpawindow;

struct dpawindow_type {
  const char* name;
  bool is_workspace;
  struct xev_event_lookup_table event_lookup_table;
};

#define DPAW_SUPPORTED_WM_PROTOCOLS \
  X(WM_TAKE_FOCUS) \
  X(WM_DELETE_WINDOW)

struct dpawindow {
  const struct dpawindow_type* type;
  struct dpaw* dpaw;
  struct dpaw_list_entry dpaw_window_entry;
  struct dpaw_list_entry dpaw_window_update_entry;
  Window xwindow;
  struct dpaw_rect boundary;
  bool mapped : 1;
  bool hidden : 1;
  bool d_update_config : 1;
  struct {
#define X(Y) bool Y : 1;
  DPAW_SUPPORTED_WM_PROTOCOLS
#undef X
  } WM_PROTOCOLS;
};

#define EV_ON(TYPE, EVENT) \
  enum event_handler_result dpaw_ev_on__ ## TYPE ## __ ## EVENT ( \
    struct dpawindow_  ## TYPE* window, \
    const struct xev_event* xev, \
    xev_ ## EVENT ## _t* event \
  ); \
  __attribute__((used,constructor(1010))) \
  void dpaw_ev_init__ ## TYPE ## __ ## EVENT (void) { \
    extern struct dpawindow_type dpawindow_type_ ## TYPE; \
    if(dpaw_xev_set_event_handler( \
      &dpawindow_type_ ## TYPE.event_lookup_table, \
      &dpaw_xev_ev2ext_ ## EVENT, \
      (dpaw_event_handler_t)dpaw_ev_on__ ## TYPE ## __ ## EVENT \
    )) _Exit(1); \
  } \
  enum event_handler_result dpaw_ev_on__ ## TYPE ## __ ## EVENT ( \
    UNUSED_ATTR struct dpawindow_  ## TYPE* window, \
    UNUSED_ATTR const struct xev_event* xev, \
    UNUSED_ATTR xev_ ## EVENT ## _t* event \
  )

#define DECLARE_DPAW_DERIVED_WINDOW(NAME, ...) \
  struct dpawindow_ ## NAME { \
    struct dpawindow window; /* Must be the first member */ \
    __VA_ARGS__ \
  }; \
  extern struct dpawindow_type dpawindow_type_ ## NAME; \
  int dpawindow_ ## NAME ## _init_super(struct dpaw*, struct dpawindow_ ## NAME*); \
  int dpawindow_ ## NAME ## _cleanup_super(struct dpawindow_ ## NAME*);

#define DEFINE_DPAW_DERIVED_WINDOW(NAME) \
  struct dpawindow_type dpawindow_type_ ## NAME = { \
    .name = #NAME \
  }; \
  __attribute__((constructor)) \
  static void dpawindow_type_constructor_ ## NAME(void){ \
    dpawindow_type_ ## NAME.is_workspace = !strncmp("workspace_", #NAME, 10); \
  } \
  int dpawindow_ ## NAME ## _init_super(struct dpaw* dpaw, struct dpawindow_ ## NAME* w){ \
    w->window.type = &dpawindow_type_ ## NAME; \
    w->window.dpaw = dpaw; \
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
struct dpawindow* dpawindow_lookup(struct dpaw*, Window);
enum event_handler_result dpawindow_dispatch_event(struct dpawindow* window, const struct xev_event*);
int dpawindow_deferred_update(struct dpawindow* window);
int dpawindow_hide(struct dpawindow* window, bool hidden);
int dpawindow_set_mapping(struct dpawindow* window, bool mapping);
int dpawindow_place_window(struct dpawindow*, struct dpaw_rect boundary);
int dpawindow_register(struct dpawindow* window);
int dpawindow_unregister(struct dpawindow* window);
int dpawindow_close(struct dpawindow* window);

#endif
