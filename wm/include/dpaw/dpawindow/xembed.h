#ifndef DPAWINDOW_XEMBED_H
#define DPAWINDOW_XEMBED_H

#include <dpaw/dpawindow/app.h>
#include <dpaw/process.h>


enum dpaw_xembed_exec_id_exchange_method {
  XEMBED_UNSUPPORTED_TAKE_FIRST_WINDOW,
  XEMBED_METHOD_GIVE_WINDOW_BY_ARGUMENT,
  XEMBED_METHOD_TAKE_WINDOW_FROM_STDOUT,
};

typedef struct dpawindow_xembed dpawindow_xembed;
DPAW_DECLARE_CALLBACK_TYPE(dpawindow_xembed)

DECLARE_DPAW_DERIVED_WINDOW( xembed,
  struct dpawindow_app parent;
  struct dpaw_process process;
  struct dpaw_callback_list_dpawindow_xembed window_changed;
  struct dpaw_callback_dpawindow pre_cleanup;
  struct dpaw_callback_dpawindow parent_boundary_changed;
  struct dpaw_callback_dpawindow_root new_window;
  int stdout;
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
