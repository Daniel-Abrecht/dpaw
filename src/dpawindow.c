#include <dpawin.h>
#include <dpawindow.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

extern bool dpawin_xerror_occured;
bool dpawin_xerror_occured;

bool dpawindow_has_error_occured(Display* display){
  XSync(display, false);
  bool result = dpawin_xerror_occured;
  dpawin_xerror_occured = false;
  return result;
}

struct dpawindow* dpawindow_lookup(struct dpawin* dpawin, Window window){
  for(struct dpawin_list_entry* it = dpawin->window_list.first; it; it=it->next){
    struct dpawindow* wit = container_of(it, struct dpawindow, dpawin_window_entry);
    if(wit->xwindow == window)
      return wit;
  }
  return 0;
}

enum event_handler_result dpawindow_dispatch_event(struct dpawindow* window, struct xev_event* event){
  return dpawin_xev_dispatch(&window->type->event_lookup_table, window, event);
}

int dpawindow_hide(struct dpawindow* window, bool hidden){
  if(!hidden && window->mapped){
    XMapWindow(window->dpawin->root.display, window->xwindow);
  }else{
    XUnmapWindow(window->dpawin->root.display, window->xwindow);
  }
  window->hidden = hidden;
  return 0;
}

int dpawindow_set_mapping(struct dpawindow* window, bool mapping){
  if(mapping && !window->hidden){
    XMapWindow(window->dpawin->root.display, window->xwindow);
  }else{
    XUnmapWindow(window->dpawin->root.display, window->xwindow);
  }
  window->mapped = mapping;
  return 0;
}

int dpawindow_place_window(struct dpawindow* window, struct dpawin_rect boundary){
  if( boundary.top_left.x >= boundary.bottom_right.x
   || boundary.top_left.y >= boundary.bottom_right.y
  ){
    memset(&boundary, 0, sizeof(boundary));
    XUnmapWindow(window->dpawin->root.display, window->xwindow);
  }else{
    bool is_visible = window->mapped && !window->hidden;
    if(!is_visible)
      XUnmapWindow(window->dpawin->root.display, window->xwindow);
    XMoveResizeWindow(
      window->dpawin->root.display,
      window->xwindow,
      boundary.top_left.x,
      boundary.top_left.y,
      boundary.bottom_right.x - boundary.top_left.x,
      boundary.bottom_right.y - boundary.top_left.y
    );
    if(is_visible)
      XMapWindow(window->dpawin->root.display, window->xwindow);
  }
  window->boundary = boundary;
  return 0;
}

int dpawindow_register(struct dpawindow* window){
  if(!window || !window->type || window->dpawin_window_entry.list)
    return -1;
  bool isroot = !strcmp(window->type->name, "root");
  if(isroot != !window->dpawin->window_list.first)
    return -1;
  for(struct xev_event_extension* extension=dpawin_event_extension_list; extension; extension=extension->next){
    if(!extension->initialised)
      continue;
    if(!extension->listen)
      continue;
    if(extension->listen(extension, window)){
      fprintf(stderr, "Failed to listen for events of extension %s on window of type %s\n", extension->name, window->type->name);
      if(extension->required)
        return -1;
    }
  }
  dpawin_linked_list_set(&window->dpawin->window_list, &window->dpawin_window_entry, 0);
  return 0;
}

int dpawindow_unregister(struct dpawindow* window){
  if(!window || !window->dpawin)
    return -1;
  dpawin_linked_list_set(0, &window->dpawin_window_entry, 0);
  return 0;
}
