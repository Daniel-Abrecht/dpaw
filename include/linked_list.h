#ifndef DPAWIN_LINKED_LIST
#define DPAWIN_LINKED_LIST

#include <dpawin_macros.h>
#include <stdbool.h>

struct dpawin_list {
  struct dpawin_list_entry *first, *last;
};

struct dpawin_list_entry {
  struct dpawin_list* list;
  struct dpawin_list_entry *previous, *next;
};

bool dpawin_linked_list_set(
  struct dpawin_list* list,
  struct dpawin_list_entry* entry,
  struct dpawin_list_entry* before
);

#define container_of(ptr, type, member) \
  ((type*)( (char*)(ptr) - offsetof(type, member) ))

#endif
