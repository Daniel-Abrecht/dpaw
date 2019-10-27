#include <dpawin.h>
#include <dpawindow.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

static struct dpawindow *first, *last;

enum event_handler_result dpawindow_dispatch_event(struct dpawindow* window, int extension, int type, void* event){
  const struct xev_event_extension* ext = dpawin_get_event_extension(window->dpawin, extension);
  if(!ext)
    return EHR_INVALID;
  return dpawin_xev_dispatch(window->type->extension_lookup_table_list, ext, type, window, event);
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
  printf("dpawindow_register %p\n", (void*)window);
  if(!window || !window->type || window->next || window->prev)
    return -1;
  bool isroot = !strcmp(window->type->name, "root");
  if(isroot != !first)
    return -1;
  printf("dpawindow_register...\n");
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
  printf("dpawindow_unregister %p\n", (void*)window);
  if(!window)
    return -1;
  if(!window->prev && !window->next && first)
    return -1;
  printf("dpawindow_unregister...\n");
  if(window->prev)
    window->prev->next = window->next;
  if(window->next)
    window->next->prev = window->prev;
  if(window == first)
    first = window->next;
  if(window == last)
    last = window->prev;
  window->prev = 0;
  window->next = 0;
  return 0;
}
