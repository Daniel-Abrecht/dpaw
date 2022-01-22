#include <-dpaw/font.h>

struct dpaw_font dpaw_font;

int dpaw_font_init(Display* display){
  char** missing_charset_list_return;
  int missing_charset_count_return;
  char* def_string_return;
  dpaw_font.fixed  = XCreateFontSet(display, "fixed", &missing_charset_list_return, &missing_charset_count_return, &def_string_return);
  dpaw_font.normal = XCreateFontSet(display, "-*-times-medium-r-*-*-*-100-*-*-*-*-*-*", &missing_charset_list_return, &missing_charset_count_return, &def_string_return);
  dpaw_font.bold   = XCreateFontSet(display, "-*-times-bold-r-*-*-*-100-*-*-*-*-*-*", &missing_charset_list_return, &missing_charset_count_return, &def_string_return);
  return 0;
}
