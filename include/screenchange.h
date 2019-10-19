#ifndef SCREENCHANGE_H
#define SCREENCHANGE_H

#include <primitives.h>

enum dpawin_screenchange_type {
  DPAWIN_SCREENCHANGE_SCREEN_ADDED,
  DPAWIN_SCREENCHANGE_SCREEN_REMOVED,
  DPAWIN_SCREENCHANGE_SCREEN_CHANGED,
};

struct dpawin_screen_info {
  struct dpawin_rect boundary;
};

typedef void (*dpawin_screenchange_handler_t)(void*, enum dpawin_screenchange_type, const struct dpawin_screen_info*);

int dpawin_screenchange_check(void);
int dpawin_screenchange_listener_register(dpawin_screenchange_handler_t, void*);
int dpawin_screenchange_listener_unregister(dpawin_screenchange_handler_t, void*);

#endif
