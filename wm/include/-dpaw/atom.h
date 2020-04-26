#ifndef ATOM_H
#define ATOM_H

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <stddef.h>

struct dpaw_atom {
  Atom* atom;
  const char* name;
  struct dpaw_atom* next;
};

extern struct dpaw_atom* dpaw_atom_list;

struct dpawindow;

int dpaw_atom_init(Display* display);
int dpaw_get_property(struct dpawindow* win, Atom property, size_t* size, size_t min_size, void** result);

#endif
