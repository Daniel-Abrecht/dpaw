#ifndef XEV_H
#define XEV_H

#include <stddef.h>
#include <stdbool.h>
#include <X11/Xlib.h>

enum { XEV_BaseEvent = -1 };

enum event_handler_result {
  EHR_FATAL_ERROR = -1,
  EHR_OK = 0,
  EHR_ERROR = 1,
  EHR_INVALID = 2,
  EHR_UNHANDLED = 3,
  EHR_NEXT = 4,
};

struct dpawindow;
typedef enum event_handler_result (*dpaw_event_handler_t)(struct dpawindow*, void*);

struct dpaw;
struct xev_event_extension;

struct xev_event {
  const struct xev_event_info* info;
  struct dpaw* dpaw;
  XAnyEvent* event;
  void* data;
  void* data_list[12];
  unsigned char index;
};

struct xev_event_specializer {
  int type_offset;
  int type_size;
  int extension_offset;
  int extension_size;
  void(*load_data)(struct dpaw* dpaw, void**);
  void(*free_data)(struct dpaw* dpaw, void*);
};

struct xev_event_info {
  struct xev_event_list* event_list;
  struct xev_event_list* children;
  char* name;
  int type;
  size_t index;
  const struct xev_event_specializer* spec;
};

struct xev_event_list {
  struct xev_event_info* parent_event;
  const struct xev_event_list* next; // A different extension may have events for the same parent event type
  const struct xev_event_extension* extension;
  struct xev_event_info*const* list;
  size_t size, index_size;
  size_t handler_list_index;
  int first_event;
};

struct xev_event_extension {
  struct xev_event_extension* next;
  bool initialised;
  bool required;
  const char* name;
  struct xev_event_list*const* event_list;
  size_t event_list_size;
  int opcode;
  int first_error;
  int(*init)(struct dpaw*, struct xev_event_extension*);
  int(*cleanup)(struct dpaw*, struct xev_event_extension*);
  int(*listen)(struct xev_event_extension*, struct dpawindow*);
  void(*preprocess_event)(struct dpaw*, XEvent*);
  int(*dispatch)(struct dpaw*, struct xev_event*);
};

struct dpaw_event_handler {
  dpaw_event_handler_t callback;
  const struct xev_event_info* info;
};

struct dpaw_event_handler_list {
  struct xev_event_list* event_list;
  struct dpaw_event_handler* handler;
};

struct xev_event_lookup_table {
  struct dpaw_event_handler_list* event_handler_list;
};

extern size_t dpaw_handler_list_count; // Number of all lists of all extensions together
extern struct xev_event_extension* dpaw_event_extension_list;

int dpaw_xev_set_event_handler(struct xev_event_lookup_table*, const struct xev_event_info*, dpaw_event_handler_t);
enum event_handler_result dpaw_xev_dispatch(const struct xev_event_lookup_table*, struct dpawindow*, struct xev_event*);
int dpaw_xevent_to_xev(struct dpaw* dpaw, struct xev_event*, XAnyEvent* event);
void dpaw_free_xev(struct xev_event*);

#endif
