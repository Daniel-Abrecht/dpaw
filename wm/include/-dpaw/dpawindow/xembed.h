#ifndef DPAWINDOW_XEMBED_H
#define DPAWINDOW_XEMBED_H

#include <-dpaw/dpawindow/app.h>
#include <-dpaw/process.h>

/* Although the spec says 0, it seams everyone uses 1 instead. */
enum { DPAW_XEMBED_VERSION = 1 };

//// Stuff defined in xembed spec ////

enum xembed_flag {
  XEMBED_MAPPED = 1<<0,
};

enum xembed_message {
  XEMBED_EMBEDDED_NOTIFY,
  XEMBED_WINDOW_ACTIVATE,
  XEMBED_WINDOW_DEACTIVATE,
  XEMBED_REQUEST_FOCUS,
  XEMBED_FOCUS_IN,
  XEMBED_FOCUS_OUT,
  XEMBED_FOCUS_NEXT,
  XEMBED_FOCUS_PREV,
  XEMBED_GRAB_KEY, // obsolete
  XEMBED_UNGRAB_KEY, // obsolete
  XEMBED_MODALITY_ON,
  XEMBED_MODALITY_OFF,
  XEMBED_REGISTER_ACCELERATOR,
  XEMBED_UNREGISTER_ACCELERATOR,
  XEMBED_ACTIVATE_ACCELERATOR,
};

enum xembed_accelerator_modifier {
  XEMBED_MODIFIER_SHIFT   = 1<<0,
  XEMBED_MODIFIER_CONTROL = 1<<1,
  XEMBED_MODIFIER_ALT     = 1<<2,
  XEMBED_MODIFIER_SUPER   = 1<<3,
  XEMBED_MODIFIER_HYPER   = 1<<4,
};

enum xembed_accelerator_flags {
  XEMBED_ACCELERATOR_OVERLOADED = 1<<0
};

enum xembed_focus_direction {
  XEMBED_DIRECTION_DEFAULT,
  XEMBED_DIRECTION_UP_DOWN,
  XEMBED_DIRECTION_LEFT_RIGHT,
};

enum xembed_focus_in_detail {
  XEMBED_FOCUS_CURRENT,
  XEMBED_FOCUS_FIRST,
  XEMBED_FOCUS_LAST,
};

////  other stuff  ////

enum dpaw_xembed_exec_id_exchange_method {
  XEMBED_UNSUPPORTED_TAKE_FIRST_WINDOW,
  XEMBED_METHOD_GIVE_WINDOW_BY_ARGUMENT,
  XEMBED_METHOD_TAKE_WINDOW_FROM_STDOUT,
};

typedef struct dpawindow_xembed dpawindow_xembed;
DPAW_DECLARE_CALLBACK_TYPE(dpawindow_xembed)

struct dpaw_xembed_info {
  long version;
  long flags;
};

DECLARE_DPAW_DERIVED_WINDOW( xembed,
  struct dpawindow_app parent;
  struct dpaw_process process;
  struct dpaw_callback_list_dpawindow_xembed window_changed;
  struct dpaw_callback_dpawindow pre_cleanup;
  struct dpaw_callback_dpawindow parent_boundary_changed;
  struct dpaw_callback_dpawindow_root new_window;
  struct dpaw_xembed_info info;
  int stdout;
  bool ready;
)

int dpawindow_xembed_init(
  struct dpaw* dpaw,
  struct dpawindow_xembed* xembed
);

int dpawindow_xembed_exec_v(
  struct dpawindow_xembed* xembed,
  enum dpaw_xembed_exec_id_exchange_method exmet,
  struct dpaw_process_create_options options
);
#define dpawindow_xembed_exec(XEMBED, EXMET, ...) \
  dpawindow_xembed_exec_v(XEMBED, EXMET, (const struct dpaw_process_create_options){.args=__VA_ARGS__})

int dpawindow_xembed_set(struct dpawindow_xembed*, Window);

#endif
