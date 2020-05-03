#ifndef DPAW_API_ONSCREEN_KEYBOARD_H
#define DPAW_API_ONSCREEN_KEYBOARD_H

#include <X11/Xlib.h>
#include <stdbool.h>

/**
 * Sets the _DPAW_EDITABLE atom on the specified window (focusee).
 * The Window should refer to the toplevel window in which some editable component is active.
 * This should be unrelated to wheter the toplevel window has the focus or not.
 * The main purpose of this is to allow the window manager to show an onscreen keyboard.
 * Don't forget to mark it as non-editable when there is no active editable element in the focusee anymore.
 * 
 * \param display The X11 display the focusee window belongs to.
 * \param focusee The window in which an editable element is active.
 * \param ready If there is an editable element active or not in the focusee window.
 **/
void dpaw_set_editable(Display* display, Window focusee, bool active);

#endif
