#ifndef XEV_X_H

#include <X11/extensions/XInput2.h>

#define XEV_EVENTS \
  X(XI_DeviceChanged, XIDeviceChangedEvent) \
  X(XI_KeyPress, XIDeviceEvent) \
  X(XI_KeyRelease, XIDeviceEvent) \
  X(XI_ButtonPress, XIDeviceEvent) \
  X(XI_ButtonRelease, XIDeviceEvent) \
  X(XI_Motion, XIDeviceEvent) \
  X(XI_Enter, XIEnterEvent) \
  X(XI_Leave, XILeaveEvent) \
  X(XI_FocusIn, XIFocusInEvent) \
  X(XI_FocusOut, XIFocusOutEvent) \
  X(XI_HierarchyChanged, XIHierarchyEvent) \
  X(XI_PropertyEvent, XIPropertyEvent) \
  X(XI_RawKeyPress, XIRawEvent) \
  X(XI_RawKeyRelease, XIRawEvent) \
  X(XI_RawButtonPress, XIRawEvent) \
  X(XI_RawButtonRelease, XIRawEvent) \
  X(XI_RawMotion, XIRawEvent) \
  X(XI_TouchBegin, XIDeviceEvent) \
  X(XI_TouchUpdate, XIDeviceEvent) \
  X(XI_TouchEnd, XIDeviceEvent) \
  X(XI_TouchOwnership, XITouchOwnershipEvent) \
  X(XI_RawTouchBegin, XIRawEvent) \
  X(XI_RawTouchUpdate, XIRawEvent) \
  X(XI_RawTouchEnd, XIRawEvent) \
  X(XI_BarrierHit, XIBarrierEvent) \
  X(XI_BarrierLeave, XIBarrierEvent) \

#define XEV_EXT xinput2
#define XEV_OPTIONAL

#include <xev.template>

#define EV_ON_TOUCH(TYPE) \
  enum event_handler_result dpawin_ev_on__ ## TYPE ## __touch (struct dpawindow_  ## TYPE* window, XIDeviceEvent* event); \
  EV_ON(TYPE, XI_TouchBegin){ \
    return dpawin_ev_on__ ## TYPE ## __touch(window, event); \
  } \
  EV_ON(TYPE, XI_TouchUpdate){ \
    return dpawin_ev_on__ ## TYPE ## __touch(window, event); \
  } \
  EV_ON(TYPE, XI_TouchEnd){ \
    return dpawin_ev_on__ ## TYPE ## __touch(window, event); \
  } \
  enum event_handler_result dpawin_ev_on__ ## TYPE ## __touch (struct dpawindow_  ## TYPE* window, XIDeviceEvent* event)


#endif
