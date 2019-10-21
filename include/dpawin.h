#ifndef DPAWIN_H
#define DPAWIN_H

#include <dpawindow/root.h>

struct dpawin {
  struct dpawindow_root root;
};

int dpawin_cleanup(void);
int dpawin_init(void);
int dpawin_run(void);
int dpawin_error_handler(Display*, XErrorEvent*);

extern struct dpawin dpawin;

#endif
