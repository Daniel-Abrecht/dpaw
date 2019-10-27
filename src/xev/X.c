#include <xev/X.c>

int dpawin_X_init(struct dpawin* dpawin, struct dpawin_xev* xev){
  (void)dpawin;
  // The standard X event aren't extensions, so let's use an invalid extension opcode as a sentinel.
  xev->extension = -1;
  return 0;
}

int dpawin_X_cleanup(struct dpawin_xev* xev){
  (void)xev;
  return 0;
}
