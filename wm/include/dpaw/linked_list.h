#ifndef DPAW_LINKED_LIST
#define DPAW_LINKED_LIST

#include <dpaw/dpaw_macros.h>
#include <stdbool.h>
#include <stddef.h>

struct dpaw_list {
  size_t size;
  struct dpaw_list_entry *first, *last;
};

struct dpaw_list_entry {
  struct dpaw_list* list;
  struct dpaw_list_entry *previous, *next;
};

bool dpaw_linked_list_set(
  struct dpaw_list* list,
  struct dpaw_list_entry* entry,
  struct dpaw_list_entry* before
);

bool dpaw_linked_list_move(
  struct dpaw_list* dst,
  struct dpaw_list* src,
  struct dpaw_list_entry* before
);

void dpaw_linked_list_clear(struct dpaw_list* list);

#endif
