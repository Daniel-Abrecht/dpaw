#include <xevent_aliases.h>

static const char*const event_name[LASTEvent] = {
#define X(C, T, F) [C] = #C,
  XEVENT_ALIAS_LIST
#undef X
};

const char* dpawin_get_event_name(int type, int extension){
  if(extension == -1){
    if(type < 0 || type > LASTEvent)
      return "invalid";
    if(!event_name[type])
      return "unknown";
    return event_name[type];
  }else{
    return "unknown extension";
  }
}
