#ifndef XEV_XINPUT2_H
#define XEV_XINPUT2_H

#include <X11/extensions/XInput2.h>

#define XEV_EVENTS \
  X(GenericEvent, ( \
    Y(XI_DeviceChanged, XIDeviceChangedEvent) \
    Y(XI_KeyPress, XIDeviceEvent) \
    Y(XI_KeyRelease, XIDeviceEvent) \
    Y(XI_ButtonPress, XIDeviceEvent) \
    Y(XI_ButtonRelease, XIDeviceEvent) \
    Y(XI_Motion, XIDeviceEvent) \
    Y(XI_Enter, XIEnterEvent) \
    Y(XI_Leave, XILeaveEvent) \
    Y(XI_FocusIn, XIFocusInEvent) \
    Y(XI_FocusOut, XIFocusOutEvent) \
    Y(XI_HierarchyChanged, XIHierarchyEvent) \
    Y(XI_PropertyEvent, XIPropertyEvent) \
    Y(XI_RawKeyPress, XIRawEvent) \
    Y(XI_RawKeyRelease, XIRawEvent) \
    Y(XI_RawButtonPress, XIRawEvent) \
    Y(XI_RawButtonRelease, XIRawEvent) \
    Y(XI_RawMotion, XIRawEvent) \
    Y(XI_TouchBegin, XIDeviceEvent) \
    Y(XI_TouchUpdate, XIDeviceEvent) \
    Y(XI_TouchEnd, XIDeviceEvent) \
    Y(XI_TouchOwnership, XITouchOwnershipEvent) \
    Y(XI_RawTouchBegin, XIRawEvent) \
    Y(XI_RawTouchUpdate, XIRawEvent) \
    Y(XI_RawTouchEnd, XIRawEvent) \
    Y(XI_BarrierHit, XIBarrierEvent) \
    Y(XI_BarrierLeave, XIBarrierEvent) \
  ))

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
