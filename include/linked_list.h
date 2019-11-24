#ifndef DPAW_LINKED_LIST
#define DPAW_LINKED_LIST

#include <dpaw_macros.h>
#include <stdbool.h>

struct dpaw_list {
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

#define container_of(ptr, type, member) \
  ((type*)( (char*)(ptr) - offsetof(type, member) ))

#endif
