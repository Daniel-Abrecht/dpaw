#ifndef SCREENCHANGE_H
#define SCREENCHANGE_H

#include <primitives.h>
#include <X11/Xlib.h>

enum dpawin_screenchange_type {
  DPAWIN_SCREENCHANGE_SCREEN_ADDED,
  DPAWIN_SCREENCHANGE_SCREEN_REMOVED,
  DPAWIN_SCREENCHANGE_SCREEN_CHANGED,
};

struct dpawin_screen_info {
  struct dpawin_rect boundary;
};

typedef void (*dpawin_screenchange_handler_t)(void*, enum dpawin_screenchange_type, const struct dpawin_screen_info*);

struct dpawin_screenchange_detector {
  Display* display;
  struct screenchange_listener* screenchange_listener_list;
  struct dpawin_screen_info_list_entry* screen_list;
};

int dpawin_screenchange_init(struct dpawin_screenchange_detector*, Display*);
int dpawin_screenchange_listener_register(struct dpawin_screenchange_detector*, dpawin_screenchange_handler_t, void*);
int dpawin_screenchange_listener_unregister(struct dpawin_screenchange_detector*, dpawin_screenchange_handler_t, void*);

#endif
