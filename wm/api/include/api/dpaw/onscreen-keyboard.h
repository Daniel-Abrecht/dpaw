#ifndef DPAW_API_ONSCREEN_KEYBOARD_H
#define DPAW_API_ONSCREEN_KEYBOARD_H

#include <X11/Xlib.h>
#include <stdbool.h>

void dpaw_input_ready_notify(Display* display, Window focusee, bool ready);

#endif
