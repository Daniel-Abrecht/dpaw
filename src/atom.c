#include <atom.h>
#include <stdio.h>
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
