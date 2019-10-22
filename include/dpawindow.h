#ifndef DPAWINDOW_H
#define DPAWINDOW_H

#include <X11/Xlib.h>
#include <xevent_aliases.h>

enum event_handler_result {
  EHR_FATAL_ERROR = -1,
  EHR_OK = 0,
  EHR_ERROR = 1,
  EHR_UNHANDLED = 2,
};

struct dpawindow;
typedef enum event_handler_result (*dpawin_event_handler_t)(struct dpawindow*, XEvent*);

struct dpawindow_type {
  const char* name;
  dpawin_event_handler_t event_handler_list[LASTEvent];
};

struct dpawindow {
  const struct dpawindow_type* type;
  struct dpawindow *prev, *next;
  Window xwindow;
};

#define EV_ON(TYPE, EVENT) \
  enum event_handler_result dpawin_ev_on__ ## TYPE ## __ ## EVENT (struct dpawindow_  ## TYPE* window, xev_ ## EVENT ## _t* event); \
  __attribute__((used,constructor)) \
  void dpawin_ev_init__ ## TYPE ## __ ## EVENT (void) { \
    extern struct dpawindow_type dpawindow_type_ ## TYPE; \
    typedef enum event_handler_result(*handler_type)(struct dpawindow_  ## TYPE* window, xev_ ## EVENT ## _t* event); \
    *(handler_type*)&dpawindow_type_ ## TYPE.event_handler_list[EVENT] = dpawin_ev_on__ ## TYPE ## __ ## EVENT; \
  } \
  enum event_handler_result dpawin_ev_on__ ## TYPE ## __ ## EVENT (struct dpawindow_  ## TYPE* window, xev_ ## EVENT ## _t* event)

#define DECLARE_DPAWIN_DERIVED_WINDOW(NAME, ...) \
  struct dpawindow_ ## NAME { \
    struct dpawindow window; /* Must be the first member */ \
    __VA_ARGS__ \
  }; \
  extern struct dpawindow_type dpawindow_type_ ## TYPE; \
  int dpawindow_ ## NAME ## _init_super(struct dpawindow_ ## NAME*);

#define DEFINE_DPAWIN_DERIVED_WINDOW(NAME) \
  struct dpawindow_type dpawindow_type_ ## NAME = { \
    .name = #NAME \
  }; \
  int dpawindow_ ## NAME ## _init_super(struct dpawindow_ ## NAME* w){ \
    w->window.type = &dpawindow_type_ ## NAME; \
    if(dpawindow_register(&w->window)) \
      return -1; \
    return 0; \
  }

enum event_handler_result dpawindow_dispatch_event(struct dpawindow* window, XEvent* event);
int dpawindow_register(struct dpawindow* window);
int dpawindow_unregister(struct dpawindow* window);

#endif
