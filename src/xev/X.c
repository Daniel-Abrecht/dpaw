#include <xev/X.c>
#include <dpawin.h>
#include <dpawindow.h>

int dpawin_xev_X_init(struct dpawin* dpawin, struct dpawin_xev* xev){
  (void)dpawin;
  // The standard X event aren't extensions, so let's use an invalid extension opcode as a sentinel.
  xev->extension = -1;
  return 0;
}

int dpawin_xev_X_cleanup(struct dpawin* dpawin, struct dpawin_xev* xev){
  (void)xev;
  (void)dpawin;
  return 0;
}

enum event_handler_result dpawin_xev_X_dispatch(struct dpawin* dpawin, struct dpawin_xev* xev, int event, void* data){
  if(event == GenericEvent || event == KeyPress || event == KeyRelease)
    return EHR_UNHANDLED;
  XEvent* ev = data;
  struct dpawindow* window = 0;
  for(struct dpawin_list_entry* it=dpawin->window_list.first; it; it=it->next){
    struct dpawindow* wit = container_of(it, struct dpawindow, dpawin_window_entry);
    if(ev->xany.window != wit->xwindow)
      continue;
    window = wit;
    break;
  }
  if(!window || window->dpawin_window_entry.next == dpawin->window_list.first)
    return EHR_UNHANDLED;
  return dpawindow_dispatch_event(window, xev->xev, event, data);
}

int dpawin_xev_X_listen(struct dpawin_xev* xev, struct dpawindow* window){
  unsigned long mask = 0;
  struct dpawindow* set[] = {
    window,
    &window->dpawin->root.window
  };
  for(size_t i=0; i<sizeof(set)/sizeof(*set); i++){
    struct dpawindow* it = set[i];
    struct xev_event_lookup_table* table = it->type->extension_lookup_table_list;
    if(table && table->handler){
      table += xev->xev->extension_index;
      size_t size = xev->xev->info_size;
      for(size_t j=1; j<size; j++)
        if(table->handler[j])
          mask |= 1lu << xev->xev->info[j].type;
    }
  }
  // Flush previouse errors
  dpawindow_has_error_occured(window->dpawin->root.display);
  XSelectInput(window->dpawin->root.display, window->xwindow, mask);
  return dpawindow_has_error_occured(window->dpawin->root.display);
}
