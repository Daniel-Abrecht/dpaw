#ifndef SCREENCHANGE_H
#define SCREENCHANGE_H

#include <primitives.h>
#include <X11/Xlib.h>

enum dpaw_screenchange_type {
  DPAW_SCREENCHANGE_SCREEN_ADDED,
  DPAW_SCREENCHANGE_SCREEN_REMOVED,
  DPAW_SCREENCHANGE_SCREEN_CHANGED,
};

struct dpaw_screen_info {
  struct dpaw_rect boundary;
  struct dpaw_point physical_size_mm;
  const char* name;
};

typedef void (*dpaw_screenchange_handler_t)(void*, enum dpaw_screenchange_type, const struct dpaw_screen_info*);

struct dpaw_screenchange_detector {
  struct dpaw* dpaw;
  struct screenchange_listener* screenchange_listener_list;
  struct dpaw_screen_info_list_entry* screen_list;
  struct dpaw_xrandr_private* xrandr;
};

int dpaw_screenchange_init(struct dpaw_screenchange_detector*, struct dpaw*);
void dpaw_screenchange_destroy(struct dpaw_screenchange_detector*);
int dpaw_screenchange_listener_register(struct dpaw_screenchange_detector*, dpaw_screenchange_handler_t, void*);
int dpaw_screenchange_listener_unregister(struct dpaw_screenchange_detector*, dpaw_screenchange_handler_t, void*);

#endif
