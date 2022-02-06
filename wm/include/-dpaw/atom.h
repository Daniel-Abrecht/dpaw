#ifndef ATOM_H
#define ATOM_H

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <stddef.h>
#include <stdbool.h>

struct dpaw_atom {
  Atom* atom;
  const char* name;
  struct dpaw_atom* next;
};

extern struct dpaw_atom* dpaw_atom_list;

struct dpawindow;

int dpaw_atom_init(Display* display);
int dpaw_get_property(struct dpawindow* win, Atom property, size_t* size, size_t min_size, void** result);
int dpaw_get_xwindow_property(Display* display, Window xwindow, Atom property, size_t* size, size_t min_size, void** result);
bool dpaw_has_property(struct dpawindow* win, Atom property);
bool dpaw_has_xwindow_property(Display* display, Window xwindow, Atom property);

#endif
