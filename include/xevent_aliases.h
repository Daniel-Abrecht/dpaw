#ifndef XEVENT_ALIASES
#define XEVENT_ALIASES

#define XEVENT_ALIAS_LIST \
  X(KeyPress, XKeyPressedEvent, xkey) \
  X(KeyRelease, XKeyReleasedEvent, xkey) \
  X(ButtonPress, XButtonPressedEvent, xbutton) \
  X(ButtonRelease, XButtonReleasedEvent, xbutton) \
  X(MotionNotify, XMotionEvent, xmotion) \
  X(EnterNotify, XEnterWindowEvent, xcrossing) \
  X(LeaveNotify, XLeaveWindowEvent, xcrossing) \
  X(FocusIn, XFocusInEvent, xfocus) \
  X(FocusOut, XFocusOutEvent, xfocus) \
  X(KeymapNotify, XKeymapEvent, xkeymap) \
  X(Expose, XExposeEvent, xexpose) \
  X(GraphicsExpose, XGraphicsExposeEvent, xgraphicsexpose) \
  X(NoExpose, XNoExposeEvent, xnoexpose) \
  X(VisibilityNotify, XVisibilityEvent, xvisibility) \
  X(CreateNotify, XCreateWindowEvent, xcreatewindow) \
  X(DestroyNotify, XDestroyWindowEvent, xdestroywindow) \
  X(UnmapNotify, XUnmapEvent, xunmap) \
  X(MapNotify, XMapEvent, xmap) \
  X(MapRequest, XMapRequestEvent, xmaprequest) \
  X(ReparentNotify, XReparentEvent, xreparent) \
  X(ConfigureNotify, XConfigureEvent, xconfigure) \
  X(ConfigureRequest, XConfigureRequestEvent, xconfigurerequest) \
  X(GravityNotify, XGravityEvent, xgravity) \
  X(ResizeRequest, XResizeRequestEvent, xresizerequest) \
  X(CirculateNotify, XCirculateEvent, xcirculate) \
  X(CirculateRequest, XCirculateRequestEvent, xcirculaterequest) \
  X(PropertyNotify, XPropertyEvent, xproperty) \
  X(SelectionClear, XSelectionClearEvent, xselectionclear) \
  X(SelectionRequest, XSelectionRequestEvent, xselectionrequest) \
  X(SelectionNotify, XSelectionEvent, xselection) \
  X(ColormapNotify, XColormapEvent, xcolormap) \
  X(ClientMessage, XClientMessageEvent, xclient) \
  X(MappingNotify, XMappingEvent, xmapping)

#define X(C, T, F) typedef T xev_ ## C ## _t;
  XEVENT_ALIAS_LIST // Define event types consistent with constant names
#undef X

#endif
