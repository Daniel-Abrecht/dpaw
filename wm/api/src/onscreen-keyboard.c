#include <api/dpaw/onscreen-keyboard.h>
#include <X11/Xatom.h>

#define ATOMS \
  X(_DPAW_EDITABLE)

#define X(N) \
  static Atom get_atom_ ## N(Display* display){ \
    static Atom atom = 0; \
    if(!atom) \
      atom = XInternAtom(display, #N, false); \
    return atom; \
  }
ATOMS
#undef X

void dpaw_set_editable(Display* display, Window focusee, bool active){
  XChangeProperty(display, focusee, get_atom__DPAW_EDITABLE(display), XA_CARDINAL, 32, PropModeReplace, (void*)(long[]){active}, 1);
}
