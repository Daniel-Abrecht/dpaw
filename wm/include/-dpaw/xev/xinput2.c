#ifndef XEV_XINPUT2_H
#define XEV_XINPUT2_H

#include <stdbool.h>
#include <X11/extensions/XInput2.h>
#include <-dpaw/linked_list.h>

struct dpaw_touch_event;

#define XEV_EVENTS \
  X(GenericEvent, ( \
    Y(XI_DeviceChanged, XIDeviceChangedEvent, 0) \
    Y(XI_KeyPress, XIDeviceEvent, 0) \
    Y(XI_KeyRelease, XIDeviceEvent, 0) \
    Y(XI_ButtonPress, XIDeviceEvent, 0) \
    Y(XI_ButtonRelease, XIDeviceEvent, 0) \
    Y(XI_Motion, XIDeviceEvent, 0) \
    Y(XI_Enter, XIEnterEvent, 0) \
    Y(XI_Leave, XILeaveEvent, 0) \
    Y(XI_FocusIn, XIFocusInEvent, 0) \
    Y(XI_FocusOut, XIFocusOutEvent, 0) \
    Y(XI_HierarchyChanged, XIHierarchyEvent, 0) \
    Y(XI_PropertyEvent, XIPropertyEvent, 0) \
    Y(XI_RawKeyPress, XIRawEvent, 0) \
    Y(XI_RawKeyRelease, XIRawEvent, 0) \
    Y(XI_RawButtonPress, XIRawEvent, 0) \
    Y(XI_RawButtonRelease, XIRawEvent, 0) \
    Y(XI_RawMotion, XIRawEvent, 0) \
    Y(XI_TouchBegin, struct dpaw_touch_event, 0) \
    Y(XI_TouchUpdate, struct dpaw_touch_event, 0) \
    Y(XI_TouchEnd, struct dpaw_touch_event, 0) \
    Y(XI_TouchOwnership, XITouchOwnershipEvent, 0) \
    Y(XI_RawTouchBegin, XIRawEvent, 0) \
    Y(XI_RawTouchUpdate, XIRawEvent, 0) \
    Y(XI_RawTouchEnd, XIRawEvent, 0) \
    Y(XI_BarrierHit, XIBarrierEvent, 0) \
    Y(XI_BarrierLeave, XIBarrierEvent, 0) \
  ))

#define XEV_EXT xinput2
#define XEV_OPTIONAL

#include <-dpaw/xev.template>

#define EV_ON_TOUCH(TYPE) \
  enum event_handler_result dpaw_ev_on__ ## TYPE ## __touch (struct dpawindow_  ## TYPE* window, struct dpaw_touch_event* event); \
  EV_ON(TYPE, XI_TouchBegin){ \
    return dpaw_ev_on__ ## TYPE ## __touch(window, event); \
  } \
  EV_ON(TYPE, XI_TouchUpdate){ \
    return dpaw_ev_on__ ## TYPE ## __touch(window, event); \
  } \
  EV_ON(TYPE, XI_TouchEnd){ \
    return dpaw_ev_on__ ## TYPE ## __touch(window, event); \
  } \
  enum event_handler_result dpaw_ev_on__ ## TYPE ## __touch (struct dpawindow_  ## TYPE* window, struct dpaw_touch_event* event)

struct dpaw_touch_event {
  XIDeviceEvent event; // Must be the first member
  int touch_source;
  struct dpaw_touchevent_window_map* twm;
};

enum dpaw_input_device_type {
  DPAW_INPUT_DEVICE_TYPE_MASTER_KEYBOARD,
  DPAW_INPUT_DEVICE_TYPE_MASTER_POINTER,
  DPAW_INPUT_DEVICE_TYPE_KEYBOARD,
  DPAW_INPUT_DEVICE_TYPE_POINTER,
};

struct dpaw_input_device {
  enum dpaw_input_device_type type;
  int deviceid;
  const char *name;
  bool enabled;
};

struct dpaw_input_master_device {
  struct dpaw_list_entry id_master_entry;
  struct dpaw_input_device pointer;
  struct dpaw_input_device keyboard;
  struct dpaw_list keyboard_list;
  struct dpaw_list pointer_list;

  struct dpaw_list_entry drag_event_owner;
  void* drag_event_owner_private;
};

struct dpaw_input_keyboard_device {
  struct dpaw_list_entry id_keyboard_entry;
  struct dpaw_list imd_keyboard_entry;
  struct dpaw_input_device device;
};

enum dpaw_input_pointer_type {
  DPAW_INPUT_POINTER_TYPE_UNKNOWN,
  DPAW_INPUT_POINTER_TYPE_MOUSE,
  DPAW_INPUT_POINTER_TYPE_TABLET,
  DPAW_INPUT_POINTER_TYPE_TOUCH,
};

struct dpaw_input_drag_event_owner {
  struct dpaw_list device_owner_list;
  const struct dpaw_input_drag_event_handler* handler;
};

struct dpaw_input_pointer_device {
  struct dpaw_list_entry id_pointer_entry;
  struct dpaw_list imd_pointer_entry;
  struct dpaw_input_device device;
  enum dpaw_input_pointer_type type;
};

struct dpaw_input_drag_event_handler {
  void(*onmove)(struct dpaw_input_drag_event_owner* owner, struct dpaw_input_master_device* device, const xev_XI_Motion_t* event);
  void(*ondrop)(struct dpaw_input_drag_event_owner* owner, struct dpaw_input_master_device* device, const xev_XI_ButtonRelease_t* event);
};

int dpaw_own_drag_event(struct dpaw* dpaw, const xev_XI_ButtonRelease_t* event, struct dpaw_input_drag_event_owner* owner);

#endif
