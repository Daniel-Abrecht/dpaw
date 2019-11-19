#include <xev/X.c>
#include <dpawin.h>
#include <stdint.h>
#include <stdio.h>

size_t dpawin_handler_list_count;
struct xev_event_extension* dpawin_event_extension_list;

int dpawin_xev_set_event_handler(struct xev_event_lookup_table* lookup_table, const struct xev_event_info* event, dpawin_event_handler_t callback){
  if(!event || !callback ||!lookup_table){
    fprintf(stderr, "dpawin_xev_set_event_handler: invalid arguments\n");
    return -1;
  }
  if(!lookup_table->event_handler_list){
    void* mem = calloc(sizeof(*lookup_table->event_handler_list),dpawin_handler_list_count);
    if(!mem){
      perror("calloc failed");
      return -1;
    }
    lookup_table->event_handler_list = mem;
  }
  size_t index = event->event_list->handler_list_index;
  assert(index < dpawin_handler_list_count);
  struct dpawin_event_handler_list* list = &lookup_table->event_handler_list[index];
  if(!list->handler){
    void* mem = calloc(sizeof(*list->handler), event->event_list->index_size);
    if(!mem){
      perror("calloc failed");
      return -1;
    }
    list->event_list = event->event_list;
    list->handler = mem;
  }
  assert(event->index < event->event_list->index_size);
  struct dpawin_event_handler* handler = &list->handler[event->index];
  handler->callback = callback;
  handler->info = event;
  return 0;
}

enum event_handler_result dpawin_xev_dispatch(const struct xev_event_lookup_table* lookup_table, struct dpawindow* window, struct xev_event* event){
  if(!lookup_table || !lookup_table->event_handler_list || !event || !event->info || !event->data || !window)
    return EHR_UNHANDLED;
  size_t index = event->info->event_list->handler_list_index;
  assert(index < dpawin_handler_list_count);
  struct dpawin_event_handler_list* list = &lookup_table->event_handler_list[index];
  if(!list->handler)
    return EHR_UNHANDLED;
  assert(event->info->index < event->info->event_list->index_size);
  struct dpawin_event_handler* handler = &list->handler[event->info->index];
  if(!handler->callback || !handler->info)
    return EHR_UNHANDLED;
  return handler->callback(window, event->data);
}


static inline int64_t get_int(size_t offset, size_t size, void* data){
  void* type = (char*)data + offset;
  switch(size){
    case 1: return *(uint8_t *)type; break;
    case 2: return *(uint16_t*)type; break;
    case 4: return *(uint32_t*)type; break;
    case 8: return *(uint64_t*)type; break;
  }
  return -1;
}

int dpawin_xevent_to_xev(struct dpawin* dpawin, struct xev_event* xev, XAnyEvent* xevent){
  const struct xev_event_info* evi = &dpawin_xev_ev2ext_XEV_BaseEvent;
  void* data = xevent;
  int index = -1;

  memset(xev, 0, sizeof(*xev));

  bool found;
  do {
    if(!evi->spec)
      break;
    found = false;
    if(index >= (int)(sizeof(xev->data_list)/sizeof(*xev->data_list))-1){
      while(index > -1){
        evi = evi->event_list->parent_event;
        if(evi->spec->free_data)
          evi->spec->free_data(dpawin, &xev->data_list[index]);
        index -= 1;
      }
      return -1;
    }
    xev->data_list[++index] = data;
    if(evi->spec->load_data)
      evi->spec->load_data(dpawin, &data);
    int type = get_int(evi->spec->type_offset, evi->event_list->parent_event->spec->type_size, data);
    int extension = get_int(evi->spec->extension_offset, evi->event_list->parent_event->spec->extension_size, data);
    if(type < 0)
      goto done;
    for(const struct xev_event_list* it=evi->children; it; it=it->next){
      if(extension >= 0 && extension != it->extension->opcode)
        continue;
      size_t first_event = it->first_event;
      if(first_event > (size_t)type)
        continue;
      if((size_t)(type - first_event + 1) > it->size)
        continue;
      if(!it->list[type - first_event + 1])
        goto done;
      evi = it->list[type - first_event + 1];
      found = true;
    }
  } while(found);

done:;
  xev->event = xevent;
  xev->data = data;
  xev->index = index;
  xev->info = evi;
  xev->dpawin = dpawin;
  return 0;
}

void dpawin_free_xev(struct xev_event* xev){
  if(!xev->dpawin)
    return;
  const struct xev_event_info* evi = xev->info;
  int index = xev->index;
  while(index > -1){
    evi = evi->event_list->parent_event;
    if(evi->spec->free_data)
      evi->spec->free_data(xev->dpawin, xev->data_list[index]);
    index -= 1;
  }
  memset(xev, 0, sizeof(*xev));
}
