#ifndef XEV_X_H
#define XEV_X_H

// Note: XEV_BaseEvent / XAnyEvent -> dpawin_xev_ev2ext_XEV_BaseEvent is the
//       implicit root of the generated event type tree used for event type lookups

#define XEV_EVENTS \
  X(XEV_BaseEvent, ( \
    Y(XEV_BaseEvent, XAnyEvent) \
    Y(KeyPress, XKeyPressedEvent) \
    Y(KeyRelease, XKeyReleasedEvent) \
    Y(ButtonPress, XButtonPressedEvent) \
    Y(ButtonRelease, XButtonReleasedEvent) \
    Y(MotionNotify, XMotionEvent) \
    Y(EnterNotify, XEnterWindowEvent) \
    Y(LeaveNotify, XLeaveWindowEvent) \
    Y(FocusIn, XFocusInEvent) \
    Y(FocusOut, XFocusOutEvent) \
    Y(KeymapNotify, XKeymapEvent) \
    Y(Expose, XExposeEvent) \
    Y(GraphicsExpose, XGraphicsExposeEvent) \
    Y(NoExpose, XNoExposeEvent) \
    Y(VisibilityNotify, XVisibilityEvent) \
    Y(CreateNotify, XCreateWindowEvent) \
    Y(DestroyNotify, XDestroyWindowEvent) \
    Y(UnmapNotify, XUnmapEvent) \
    Y(MapNotify, XMapEvent) \
    Y(MapRequest, XMapRequestEvent) \
    Y(ReparentNotify, XReparentEvent) \
    Y(ConfigureNotify, XConfigureEvent) \
    Y(ConfigureRequest, XConfigureRequestEvent) \
    Y(GravityNotify, XGravityEvent) \
    Y(ResizeRequest, XResizeRequestEvent) \
    Y(CirculateNotify, XCirculateEvent) \
    Y(CirculateRequest, XCirculateRequestEvent) \
    Y(PropertyNotify, XPropertyEvent) \
    Y(SelectionClear, XSelectionClearEvent) \
    Y(SelectionRequest, XSelectionRequestEvent) \
    Y(SelectionNotify, XSelectionEvent) \
    Y(ColormapNotify, XColormapEvent) \
    Y(ClientMessage, XClientMessageEvent) \
    Y(MappingNotify, XMappingEvent) \
    Y(GenericEvent, XGenericEvent) \
  ))

#define XEV_EXT X

#include <xev.template>

#endif
