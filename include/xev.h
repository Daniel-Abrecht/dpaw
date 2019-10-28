#ifndef XEV_H
#define XEV_H

#include <stddef.h>
#include <stdbool.h>

enum event_handler_result {
  EHR_FATAL_ERROR = -1,
  EHR_OK = 0,
  EHR_ERROR = 1,
  EHR_INVALID = 2,
  EHR_UNHANDLED = 3,
  EHR_NEXT = 4,
};

struct dpawindow;
typedef enum event_handler_result (*dpawin_event_handler_t)(struct dpawindow*, void*);

struct dpawin;
struct xev_event_extension;

struct xev_event_info {
  char* name;
  int type;
};

struct dpawin_xev;
struct xev_event_extension {
  struct xev_event_extension* next;
  const char* name;
  unsigned int* info_index;
  size_t info_index_size;
  struct xev_event_info* info;
  size_t info_size;
  int extension_index;
  bool required;
  int(*init)(struct dpawin*, struct dpawin_xev*);
  int(*cleanup)(struct dpawin*, struct dpawin_xev*);
  int(*dispatch)(struct dpawin*, struct dpawin_xev*, int event, void* data);
};

struct xev_event_lookup_table {
  const struct xev_event_extension* meta;
  dpawin_event_handler_t* handler;
};

struct dpawin_xev {
  int extension;
  const struct xev_event_extension* xev;
};

extern size_t dpawin_event_extension_count;
extern struct xev_event_extension* dpawin_event_extension_list;

int dpawin_xev_add_event_handler(struct xev_event_lookup_table**, const struct xev_event_extension*, int event, dpawin_event_handler_t);
enum event_handler_result dpawin_xev_dispatch(const struct xev_event_lookup_table*, const struct xev_event_extension*, int event, struct dpawindow*, void*);

struct dpawin_xev* dpawin_get_event_extension(const struct dpawin*, int extension);
const char* dpawin_get_extension_name(const struct dpawin*, int extension);
const struct xev_event_info* dpawin_get_event_info(const struct xev_event_extension*, int event);
const char* dpawin_get_event_name(const struct dpawin*, int extension, int event);

#endif
