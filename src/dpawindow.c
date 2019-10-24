#include <dpawin.h>
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

int dpawindow_hide(struct dpawindow* window, bool hidden){
  if(hidden == window->hidden)
    return 0;
  if(window->mapped){
    if(hidden){
      XUnmapWindow(window->dpawin->root.display, window->xwindow);
    }else{
      XMapWindow(window->dpawin->root.display, window->xwindow);
    }
  }
  window->hidden = hidden;
  return 0;
}

int dpawindow_set_mapping(struct dpawindow* window, bool mapping){
  if(mapping == window->mapped)
    return 0;
  if(!window->hidden){
    if(mapping){
      if(window->hidden)
        XMapWindow(window->dpawin->root.display, window->xwindow);
    }else{
      if(window->hidden)
        XUnmapWindow(window->dpawin->root.display, window->xwindow);
    }
  }
  window->mapped = mapping;
  return 0;
}

int dpawindow_place_window(struct dpawindow* window, struct dpawin_rect boundary){
  bool was_visible = window->mapped && !window->hidden && (window->boundary.top_left.x != 0 || window->boundary.bottom_right.x != 0);
  if( boundary.top_left.x >= boundary.bottom_right.x
   || boundary.top_left.y >= boundary.bottom_right.y
  ){
    memset(&boundary, 0, sizeof(boundary));
    if(was_visible)
      XUnmapWindow(window->dpawin->root.display, window->xwindow);
  }else{
    bool is_visible = window->mapped && !window->hidden;
    if(was_visible && !is_visible)
      XUnmapWindow(window->dpawin->root.display, window->xwindow);
    XMoveResizeWindow(
      window->dpawin->root.display,
      window->xwindow,
      boundary.top_left.x,
      boundary.top_left.y,
      boundary.bottom_right.x - boundary.top_left.x,
      boundary.bottom_right.y - boundary.top_left.y
    );
    if(!was_visible && is_visible)
      XMapWindow(window->dpawin->root.display, window->xwindow);
  }
  window->boundary = boundary;
  return 0;
}

int dpawindow_register(struct dpawindow* window){
  if(!window || !window->type || window->next || window->prev)
    return -1;
  bool isroot = !strcmp(window->type->name, "root");
  if(isroot != !first)
    return -1;
  if(!first){
    first = window;
    last = window;
  }else{
    last->next = window;
    window->prev = last;
    last = window;
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
