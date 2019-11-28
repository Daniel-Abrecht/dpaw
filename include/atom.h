#ifndef ATOM_H
#define ATOM_H

#include <X11/Xlib.h>

struct dpaw_atom {
  Atom* atom;
  const char* name;
  struct dpaw_atom* next;
};

extern struct dpaw_atom* dpaw_atom_list;

int dpaw_atom_init(Display* display);

#endif
