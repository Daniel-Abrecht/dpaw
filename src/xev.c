#include <xev.h>
#include <dpawin.h>
#include <stdio.h>

size_t dpawin_event_extension_count;
struct xev_event_extension* dpawin_event_extension_list;

struct dpawin_xev* dpawin_get_event_extension(const struct dpawin* dpawin, int extension){
  for(size_t i=0; i<dpawin_event_extension_count; i++){
    struct dpawin_xev* xev = dpawin->root.xev_list + i;
    if(xev->extension == extension)
      return xev;
  }
  return 0;
}

const char* dpawin_get_extension_name(const struct dpawin* dpawin, int extension){
  const struct dpawin_xev* xev = dpawin_get_event_extension(dpawin, extension);
  if(!xev || !xev->xev)
    return "unknown";
  return xev->xev->name;
}

const struct xev_event_info* dpawin_get_event_info(const struct xev_event_extension* extension, int event){
  if(!extension)
    return 0;
  if(event < 0 || (size_t)event >= extension->info_index_size)
    return 0;
  int index = extension->info_index[event];
  if(index <= 0 || (size_t)index >= extension->info_size)
    return 0;
  return &extension->info[index];
}

const char* dpawin_get_event_name(const struct dpawin* dpawin, int extension, int event){
  const struct dpawin_xev* xev = dpawin_get_event_extension(dpawin, extension);
  if(!xev || !xev->xev)
    return "unknown";
  const struct xev_event_info* info = dpawin_get_event_info(xev->xev, event);
  if(!info)
    return "unknown";
  return info->name;
}

int dpawin_xev_add_event_handler(struct xev_event_lookup_table** lookup_table, const struct xev_event_extension* extension, int event, dpawin_event_handler_t handler){
  if(!dpawin_event_extension_count)
    return -1; // Make sure xev_constructor is executed before dpawin_xev_add_event_handler!!!
  if(!lookup_table)
    return -1;
  if(!*lookup_table){
    struct xev_event_lookup_table* table = calloc(dpawin_event_extension_count, sizeof(struct xev_event_lookup_table));
    if(!table)
      return -1;
    size_t max_handlers = 0;
    for(const struct xev_event_extension* it = dpawin_event_extension_list; it; it=it->next){
      table[it->extension_index].meta = it;
      max_handlers += it->info_size;
    }
    dpawin_event_handler_t* handler = calloc(max_handlers, sizeof(dpawin_event_handler_t));
    if(!handler){
      free(table);
      return -1;
    }
    size_t offset = 0;
    for(const struct xev_event_extension* it = dpawin_event_extension_list; it; it=it->next){
      table[it->extension_index].handler = handler + offset;
      offset += it->info_size;
    }
    *lookup_table = table;
  }
  int index = extension->info_index[event];
  if(index <= 0 || (size_t)index >= extension->info_size)
    return -1;
  (*lookup_table)[extension->extension_index].handler[index] = handler;
  return 0;
}

enum event_handler_result dpawin_xev_dispatch(const struct xev_event_lookup_table* lookup_table, const struct xev_event_extension* extension, int event, struct dpawindow* window, void* arg){
  if(!lookup_table)
    return EHR_UNHANDLED;
  int index = extension->info_index[event];
  if(index <= 0 || (size_t)index >= extension->info_size)
    return EHR_INVALID;
  dpawin_event_handler_t handler = lookup_table[extension->extension_index].handler[index];
  if(!handler)
    return EHR_UNHANDLED;
  return handler(window, arg);
}
