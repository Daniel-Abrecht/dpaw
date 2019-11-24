#include <dpaw.h>
#include <dpawindow.h>
#include <dpawindow/root.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

extern bool dpaw_xerror_occured;
bool dpaw_xerror_occured;

bool dpawindow_has_error_occured(Display* display){
  XSync(display, false);
  bool result = dpaw_xerror_occured;
  dpaw_xerror_occured = false;
  return result;
}

struct dpawindow* dpawindow_lookup(struct dpaw* dpaw, Window window){
  for(struct dpaw_list_entry* it = dpaw->window_list.first; it; it=it->next){
    struct dpawindow* wit = container_of(it, struct dpawindow, dpaw_window_entry);
    if(wit->xwindow == window)
      return wit;
  }
  return 0;
}

enum event_handler_result dpawindow_dispatch_event(struct dpawindow* window, struct xev_event* event){
  return dpaw_xev_dispatch(&window->type->event_lookup_table, window, event);
}

int dpawindow_hide(struct dpawindow* window, bool hidden){
  if(!hidden && window->mapped){
    XMapWindow(window->dpaw->root.display, window->xwindow);
  }else{
    XUnmapWindow(window->dpaw->root.display, window->xwindow);
  }
  window->hidden = hidden;
  return 0;
}

int dpawindow_set_mapping(struct dpawindow* window, bool mapping){
  if(mapping && !window->hidden){
    XMapWindow(window->dpaw->root.display, window->xwindow);
  }else{
    XUnmapWindow(window->dpaw->root.display, window->xwindow);
  }
  window->mapped = mapping;
  return 0;
}

int dpawindow_place_window(struct dpawindow* window, struct dpaw_rect boundary){
  if( boundary.top_left.x >= boundary.bottom_right.x
   || boundary.top_left.y >= boundary.bottom_right.y
  ){
    memset(&boundary, 0, sizeof(boundary));
    XUnmapWindow(window->dpaw->root.display, window->xwindow);
  }else{
    bool is_visible = window->mapped && !window->hidden;
    if(!is_visible)
      XUnmapWindow(window->dpaw->root.display, window->xwindow);
    XMoveResizeWindow(
      window->dpaw->root.display,
      window->xwindow,
      boundary.top_left.x,
      boundary.top_left.y,
      boundary.bottom_right.x - boundary.top_left.x,
      boundary.bottom_right.y - boundary.top_left.y
    );
    if(is_visible)
      XMapWindow(window->dpaw->root.display, window->xwindow);
  }
  window->boundary = boundary;
  return 0;
}

int dpawindow_register(struct dpawindow* window){
  if(!window || !window->type || window->dpaw_window_entry.list){
    fprintf(stderr, "Procondition for dpawindow_register failed\n");
    return -1;
  }
  bool isroot = window->type == &dpawindow_type_root;
  if(isroot != !window->dpaw->window_list.first){
    fprintf(stderr, "The first window to be registred must be the root window\n");
    return -1;
  }
  {
    struct dpawindow* res = dpawindow_lookup(window->dpaw, window->xwindow);
    if(res){
      fprintf(stderr, "This %s window was already registred as an %s window\n", window->type->name, res->type->name);
      return -1; // This window
    }
  }
  for(struct xev_event_extension* extension=dpaw_event_extension_list; extension; extension=extension->next){
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
  dpaw_linked_list_set(&window->dpaw->window_list, &window->dpaw_window_entry, 0);
  return 0;
}

int dpawindow_unregister(struct dpawindow* window){
  if(!window || !window->dpaw)
    return -1;
  dpaw_linked_list_set(0, &window->dpaw_window_entry, 0);
  return 0;
}
