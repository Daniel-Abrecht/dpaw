#ifndef XEV_X_H
#define XEV_X_H

// Note: XEV_BaseEvent / XAnyEvent -> dpaw_xev_ev2ext_XEV_BaseEvent is the
//       implicit root of the generated event type tree used for event type lookups

#define XEV_EVENTS \
  X(XEV_BaseEvent, ( \
    Y(XEV_BaseEvent, XAnyEvent, 0) \
    Y(KeyPress, XKeyPressedEvent, KeyPressMask) \
    Y(KeyRelease, XKeyReleasedEvent, KeyReleaseMask) \
    Y(ButtonPress, XButtonPressedEvent, ButtonPressMask) \
    Y(ButtonRelease, XButtonReleasedEvent, ButtonReleaseMask) \
    Y(MotionNotify, XMotionEvent, PointerMotionMask) \
    Y(EnterNotify, XEnterWindowEvent, EnterWindowMask) \
    Y(LeaveNotify, XLeaveWindowEvent, LeaveWindowMask) \
    Y(FocusIn, XFocusInEvent, FocusChangeMask) \
    Y(FocusOut, XFocusOutEvent, FocusChangeMask) \
    Y(KeymapNotify, XKeymapEvent, KeymapStateMask) \
    Y(Expose, XExposeEvent, ExposureMask) \
    Y(GraphicsExpose, XGraphicsExposeEvent, ExposureMask) \
    Y(NoExpose, XNoExposeEvent, ExposureMask) \
    Y(VisibilityNotify, XVisibilityEvent, VisibilityChangeMask) \
    Y(CreateNotify, XCreateWindowEvent, StructureNotifyMask) \
    Y(DestroyNotify, XDestroyWindowEvent, StructureNotifyMask) \
    Y(UnmapNotify, XUnmapEvent, StructureNotifyMask) \
    Y(MapNotify, XMapEvent, StructureNotifyMask) \
    Y(MapRequest, XMapRequestEvent, SubstructureRedirectMask) \
    Y(ReparentNotify, XReparentEvent, StructureNotifyMask) \
    Y(ConfigureNotify, XConfigureEvent, StructureNotifyMask) \
    Y(ConfigureRequest, XConfigureRequestEvent, SubstructureRedirectMask) \
    Y(GravityNotify, XGravityEvent, StructureNotifyMask) \
    Y(ResizeRequest, XResizeRequestEvent, ResizeRedirectMask) \
    Y(CirculateNotify, XCirculateEvent, StructureNotifyMask) \
    Y(CirculateRequest, XCirculateRequestEvent, SubstructureRedirectMask) \
    Y(PropertyNotify, XPropertyEvent, PropertyChangeMask) \
    Y(SelectionClear, XSelectionClearEvent, 0) \
    Y(SelectionRequest, XSelectionRequestEvent, 0) \
    Y(SelectionNotify, XSelectionEvent, 0) \
    Y(ColormapNotify, XColormapEvent, ColormapChangeMask) \
    Y(ClientMessage, XClientMessageEvent, 0) \
    Y(MappingNotify, XMappingEvent, 0) \
    Y(GenericEvent, XGenericEvent, 0) \
  ))

#define XEV_EXT X

#include <-dpaw/xev.template>

#endif
