#include <api/dpaw/onscreen-keyboard.h>
#include <X11/Xatom.h>

#define ATOMS \
  X(_DPAW_INPUT_READY)

#define X(N) \
  static Atom get_atom_ ## N(Display* display){ \
    static Atom atom = 0; \
    if(!atom) \
      atom = XInternAtom(display, #N, false); \
    return atom; \
  }
ATOMS
#undef X

void dpaw_input_ready_notify(Display* display, Window focusee, bool ready){
  XChangeProperty(display, focusee, get_atom__DPAW_INPUT_READY(display), XA_CARDINAL, 32, PropModeReplace, (void*)(long[]){ready}, 1);
}
