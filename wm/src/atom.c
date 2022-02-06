#include <-dpaw/dpaw.h>
#include <-dpaw/atom.h>
#include <-dpaw/dpawindow.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

struct dpaw_atom* dpaw_atom_list;

int dpaw_atom_init(Display* display){
  for(struct dpaw_atom* it=dpaw_atom_list; it; it=it->next){
    Atom result = XInternAtom(display, it->name, false);
    if(result == None){
      fprintf(stderr, "XInternAtom failed\n");
      return -1;
    }
    *it->atom = result;
  }
  return 0;
}

int dpaw_get_xwindow_property(Display* display, Window xwindow, Atom property, size_t* size, size_t min_size, void** result){
  if(!result || !size)
    return -1;
  Atom type = 0;
  int format = 0;
  unsigned long nitems = 0;
  unsigned long remaining_size = 0;
  unsigned long max_size = *size;
  unsigned char* prop_ret = 0;
  XGetWindowProperty(
    display, xwindow,
    property,
    0, (max_size > 0 ? max_size+3 : 1024 * 1024) / 4,
    false,
    AnyPropertyType,
    &type,
    &format,
    &nitems,
    &remaining_size,
    &prop_ret
  );
  if(format == 0){
    *size = 0;
    *result = 0;
    return -1;
  }
  if(format != 8 && format != 16 && format != 32)
    return -1;
  if(!prop_ret){
    *result = 0;
    *size = 0;
    return 0;
  }
  size_t actual_size = format / 8 * nitems;
  if(min_size && actual_size < min_size){
    XFree(prop_ret);
    return -1;
  }
  if(!max_size){
    *size = actual_size;
    *result = prop_ret;
    return 0;
  }
  if(actual_size < max_size){
    XFree(prop_ret);
    return -1;
  }
  *result = prop_ret;
  return 0;
}

bool dpaw_has_xwindow_property(Display* display, Window xwindow, Atom property){
  Atom actualType;
  int actualFormat;
  unsigned long nItems;
  unsigned long bytesAfter;
  unsigned char* prop;

  XGetWindowProperty(
    display, xwindow,
    property,
    0, 0, false,
    AnyPropertyType,
    &actualType,
    &actualFormat,
    &nItems,
    &bytesAfter,
    &prop
  );
  if(prop) XFree(prop);
  return !!prop;
}

bool dpaw_has_property(struct dpawindow* win, Atom property){
  return dpaw_has_xwindow_property(win->dpaw->root.display, win->xwindow, property);
}

int dpaw_get_property(struct dpawindow* win, Atom property, size_t* size, size_t min_size, void** result){
  return dpaw_get_xwindow_property(win->dpaw->root.display, win->xwindow, property, size, min_size, result);
}
