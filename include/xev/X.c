#ifndef XEV_X_H
#define XEV_X_H

#include <X11/Xlib.h>

#define XEV_EVENTS \
  X(KeyPress, XKeyPressedEvent) \
  X(KeyRelease, XKeyReleasedEvent) \
  X(ButtonPress, XButtonPressedEvent) \
  X(ButtonRelease, XButtonReleasedEvent) \
  X(MotionNotify, XMotionEvent) \
  X(EnterNotify, XEnterWindowEvent) \
  X(LeaveNotify, XLeaveWindowEvent) \
  X(FocusIn, XFocusInEvent) \
  X(FocusOut, XFocusOutEvent) \
  X(KeymapNotify, XKeymapEvent) \
  X(Expose, XExposeEvent) \
  X(GraphicsExpose, XGraphicsExposeEvent) \
  X(NoExpose, XNoExposeEvent) \
  X(VisibilityNotify, XVisibilityEvent) \
  X(CreateNotify, XCreateWindowEvent) \
  X(DestroyNotify, XDestroyWindowEvent) \
  X(UnmapNotify, XUnmapEvent) \
  X(MapNotify, XMapEvent) \
  X(MapRequest, XMapRequestEvent) \
  X(ReparentNotify, XReparentEvent) \
  X(ConfigureNotify, XConfigureEvent) \
  X(ConfigureRequest, XConfigureRequestEvent) \
  X(GravityNotify, XGravityEvent) \
  X(ResizeRequest, XResizeRequestEvent) \
  X(CirculateNotify, XCirculateEvent) \
  X(CirculateRequest, XCirculateRequestEvent) \
  X(PropertyNotify, XPropertyEvent) \
  X(SelectionClear, XSelectionClearEvent) \
  X(SelectionRequest, XSelectionRequestEvent) \
  X(SelectionNotify, XSelectionEvent) \
  X(ColormapNotify, XColormapEvent) \
  X(ClientMessage, XClientMessageEvent) \
  X(MappingNotify, XMappingEvent) \
  X(GenericEvent, XGenericEvent)

#define XEV_EXT X

#include <xev.template>

#endif
