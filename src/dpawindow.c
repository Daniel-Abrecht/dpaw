#include <dpawindow.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

static struct dpawindow *first, *last;

enum event_handler_result dpawindow_dispatch_event(struct dpawindow* window, XEvent* event){
  if(event->type < 0 || event->type > LASTEvent){
    fprintf(stderr, "Got invalid event type %d!!!\n", event->type);
    return EHR_UNHANDLED;
  }
  dpawin_event_handler_t handler = window->type->event_handler_list[event->type];
  if(!handler)
    return EHR_UNHANDLED;
  return handler(window, event);
}

int dpawindow_register(struct dpawindow* window){
  if(!window || !window->type || window->next || window->prev)
    return -1;
  bool isroot = strcmp(window->type->name, "root") == 0;
  if(isroot != !first)
    return -1;
  if(!first){
    first = window;
    last = window;
  }else{
    window->prev = last;
    last->next = window;
  }
  return 0;
}

int dpawindow_unregister(struct dpawindow* window){
  if(!window || !window->type)
    return -1;
  if(!window->prev && !window->next && first)
    return -1;
  if(window->prev)
    window->prev->next = window->next;
  if(window->next)
    window->next->prev = window->prev;
  if(window->prev == first)
    first = window->next;
  if(window->next == last)
    last = window->prev;
  window->prev = 0;
  window->next = 0;
  return 0;
}
