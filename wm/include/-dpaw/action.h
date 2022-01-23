#ifndef DPAW_ACTION_H
#define DPAW_ACTION_H

#include <-dpaw/linked_list.h>

// Similar to dpaw_callback, but less heavy

struct dpaw_action;
typedef void dpaw_deferred_action_f(struct dpaw_action* ptr);
struct dpaw_action {
  struct dpaw_list_entry entry;
  dpaw_deferred_action_f* callback;
};

#endif
