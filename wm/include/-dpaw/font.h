#ifndef FONT_H
#define FONT_H

#include <X11/Xlib.h>

struct dpaw_font {
  XFontSet fixed;
  XFontSet normal;
  XFontSet bold;
};

extern struct dpaw_font dpaw_font;

int dpaw_font_init(Display* display);

#endif
