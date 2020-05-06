#include <-dpaw/dpaw.h>
#include <-dpaw/atom/dpaw.c>
#include <-dpaw/atom/icccm.c>
#include <-dpaw/dpawindow.h>
#include <-dpaw/dpawindow/root.h>
#include <X11/extensions/XRes.h>
#include <X11/Xatom.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
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

enum event_handler_result dpawindow_dispatch_event(struct dpawindow* window, const struct xev_event* event){
  return dpaw_xev_dispatch(&window->type->event_lookup_table, window, event);
}

void dpawindow_cleanup(struct dpawindow* window){
  if(!window->type)
    return;
  assert(!window->cleanup);
  window->cleanup = true;
  DPAW_CALL_BACK_AND_REMOVE(dpawindow, window, pre_cleanup, 0);
  dpaw_linked_list_clear(&window->boundary_changed.list);
  window->type->cleanup(window);
  dpawindow_unregister(window);
  window->cleanup = false;
  DPAW_CALL_BACK_AND_REMOVE(dpawindow, window, post_cleanup, 0);
}

int dpawindow_close(struct dpawindow* window){
  if(window->WM_PROTOCOLS.WM_DELETE_WINDOW){
    XSendEvent(window->dpaw->root.display, window->xwindow, false, NoEventMask, &(XEvent){.xclient={
      .type = ClientMessage,
      .window = window->xwindow,
      .message_type = WM_PROTOCOLS,
      .format = 32,
      .data.l[0] = WM_DELETE_WINDOW,
      .data.l[1] = CurrentTime
    }});
  }else{
    XKillClient(window->dpaw->root.display, window->xwindow);
  }
  return 0;
}

static void update_window_config(struct dpawindow* window){
  window->d_update_config = true;
  dpaw_linked_list_set(&window->dpaw->window_update_list, &window->dpaw_window_update_entry, 0);
}

int dpawindow_hide(struct dpawindow* window, bool hidden){
  window->hidden = hidden;
  update_window_config(window);
  return 0;
}

int dpawindow_set_mapping(struct dpawindow* window, bool mapping){
  window->mapped = mapping;
  update_window_config(window);
  return 0;
}

int dpawindow_place_window(struct dpawindow* window, struct dpaw_rect boundary){
  window->boundary = boundary;
  update_window_config(window);
  DPAW_CALL_BACK(dpawindow, window, boundary_changed, 0);
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
  {
    XWindowAttributes attrs;
    XGetWindowAttributes(window->dpaw->root.display, window->xwindow, &attrs);
    window->boundary.top_left.x = attrs.x;
    window->boundary.top_left.y = attrs.y;
    window->boundary.bottom_right.x = (long long)attrs.x + attrs.width;
    window->boundary.bottom_right.y = (long long)attrs.y + attrs.height;
  }
  {
    Atom* atoms = 0;
    size_t size = 0;
    if(!dpaw_get_property(window, WM_PROTOCOLS, &size, 0, (void**)&atoms) && size >= 4){
      size /= 4;
      for(size_t i=0; i<size; i++){
        Atom atom = atoms[i];
#define X(Y) else if(Y == atom){ window->WM_PROTOCOLS.Y = true; }
        if(0){} DPAW_SUPPORTED_WM_PROTOCOLS
#undef X
      }
      if(atoms) XFree(atoms);
    }
  }
  for(struct xev_event_extension* extension=dpaw_event_extension_list; extension; extension=extension->next){
    if(!extension->initialised)
      continue;
    if(!extension->subscribe)
      continue;
    if(extension->subscribe(extension, window)){
      fprintf(stderr, "Failed to subscribe for events of extension %s on window of type %s\n", extension->name, window->type->name);
      if(extension->required)
        return -1;
    }
  }
  dpaw_linked_list_set(&window->dpaw->window_list, &window->dpaw_window_entry, 0);
  XAddToSaveSet(window->dpaw->root.display, window->xwindow);
  return 0;
}

int dpawindow_unregister(struct dpawindow* window){
  if(!window || !window->dpaw)
    return -1;
  if(window->xwindow)
    XRemoveFromSaveSet(window->dpaw->root.display, window->xwindow);
  for(struct xev_event_extension* extension=dpaw_event_extension_list; extension; extension=extension->next)
    if(extension->unsubscribe)
      extension->unsubscribe(extension, window);
  dpaw_linked_list_set(0, &window->dpaw_window_entry, 0);
  dpaw_linked_list_set(0, &window->dpaw_window_update_entry, 0);
  return 0;
}

int dpawindow_deferred_update(struct dpawindow* window){
  if(!window->d_update_config)
    return 0;
  window->d_update_config = false;
  const struct dpaw_rect boundary = window->boundary;
  bool valid_placement = (
      boundary.top_left.x < boundary.bottom_right.x
   && boundary.top_left.y < boundary.bottom_right.y
  );
  bool is_visible = window->mapped && !window->hidden && valid_placement;
/*  printf(
    "update_window_config: %lx %ld %ld %ld %ld %c%c %c\n",
    window->xwindow,
    boundary.top_left.x, boundary.top_left.y, boundary.bottom_right.x, boundary.bottom_right.y,
    window->mapped ? 'm' : 'u',
    window->hidden ? 'h' : 'v',
    is_visible ? 'v' : 'i'
  );*/
  if(valid_placement){
    // Because of problems of resizing sometimes failing, let's try this before and after any mapping changes
    XMoveResizeWindow(
      window->dpaw->root.display,
      window->xwindow,
      boundary.top_left.x,
      boundary.top_left.y,
      boundary.bottom_right.x - boundary.top_left.x,
      boundary.bottom_right.y - boundary.top_left.y
    );
  }
  if(is_visible){
    XMapWindow(window->dpaw->root.display, window->xwindow);
  }else{
    XUnmapWindow(window->dpaw->root.display, window->xwindow);
  }
  if(valid_placement){
    XMoveResizeWindow(
      window->dpaw->root.display,
      window->xwindow,
      boundary.top_left.x,
      boundary.top_left.y,
      boundary.bottom_right.x - boundary.top_left.x,
      boundary.bottom_right.y - boundary.top_left.y
    );
  }
  long state = IconicState;
  if(!window->hidden)
    state = NormalState;
  if(!window->mapped)
    state = WithdrawnState;

  {
    // Only client windows or their frame should have the WM_STATE property.
    // Otherwise, programs such as xwininfo may get confused sometimes
    extern struct dpawindow_type dpawindow_type_app;
    Atom state_atom = window->type == &dpawindow_type_app ? WM_STATE : _DPAW_WIN_STATE;
    XChangeProperty(window->dpaw->root.display, window->xwindow, state_atom, WM_STATE, 32, PropModeReplace, (unsigned char*)(long[]){state,0}, 2);
  }

  return 0;
}

pid_t dpaw_try_get_xwindow_pid(Display* display, Window xwindow){
  pid_t pid = 0;

  XResClientIdSpec client_specs = {
    .client = xwindow,
    .mask = XRES_CLIENT_ID_PID_MASK
  };
  long num_ids = 0;
  XResClientIdValue* client_ids = 0;

  XResQueryClientIds(display, 1, &client_specs, &num_ids, &client_ids);

  for(long i=0; i<num_ids; i++){
    if(!(client_ids[i].spec.mask & XRES_CLIENT_ID_PID_MASK))
      continue;
    if(client_ids[i].length == sizeof(uint32_t)){
      uint32_t tmp;
      memcpy(&tmp, client_ids[i].value, sizeof(tmp));
      if(tmp > 0) pid = tmp;
    }
    if(client_ids[i].length == sizeof(uint64_t)){
      uint64_t tmp;
      memcpy(&tmp, client_ids[i].value, sizeof(tmp));
      if(tmp > 0) pid = tmp;
    }
    break;
  }

  if(client_ids)
    XFree(client_ids);

  return pid;
}

int dpawindow_get_reasonable_size_hints(const struct dpawindow* window, XSizeHints* ret){
  {
    XSizeHints* size_hints = XAllocSizeHints();
    if(!size_hints)
      return -1;
    long s = 0;
    int status = XGetWMNormalHints(window->dpaw->root.display, window->xwindow, size_hints, &s);
    if(!size_hints)
      return -1;
    *ret = *size_hints;
    XFree(size_hints);
    if(!status)
      return -1;
  }
  printf("min(%dx%d) base(%dx%d)\n",ret->min_width, ret->min_height, ret->base_width, ret->base_height);
  if(ret->width <= 16)
    ret->width = window->boundary.bottom_right.x - window->boundary.top_left.x;
  if(ret->height <= 16)
    ret->height = window->boundary.bottom_right.y - window->boundary.top_left.y;
  if(ret->width <= ret->min_width)
    ret->width = ret->min_width;
  if(ret->height <= ret->min_height)
    ret->height = ret->min_height;
  if(ret->width <= ret->base_width)
    ret->width = ret->base_width;
  if(ret->height <= ret->base_height)
    ret->height = ret->base_height;
  if(ret->width <= 16)
    ret->width = 600;
  if(ret->height <= 16)
    ret->height = 400;
  return 0;
}
