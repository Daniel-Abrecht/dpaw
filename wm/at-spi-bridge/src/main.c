#include <stdio.h>
#include <stddef.h>
#include <locale.h>
#include <stdbool.h>
#include <strings.h>
#include <atspi/atspi.h>

bool is_editable(AtspiRole role){
  switch(role){
    case ATSPI_ROLE_TEXT:
    case ATSPI_ROLE_TERMINAL:
    case ATSPI_ROLE_DATE_EDITOR:
    case ATSPI_ROLE_PASSWORD_TEXT:
    case ATSPI_ROLE_EDITBAR:
    case ATSPI_ROLE_ENTRY:
    case ATSPI_ROLE_DOCUMENT_TEXT:
    case ATSPI_ROLE_DOCUMENT_FRAME:
    case ATSPI_ROLE_DOCUMENT_EMAIL:
    case ATSPI_ROLE_SPIN_BUTTON:
    case ATSPI_ROLE_COMBO_BOX:
    case ATSPI_ROLE_PARAGRAPH:
    case ATSPI_ROLE_HEADER:
    case ATSPI_ROLE_FOOTER:
      return true;
    default: return false;
  }
}

AtspiEventListener* listener = 0;

static void on_event(AtspiEvent *event, void *data){
  (void)data;
  if(!event->source)
    return;

  AtspiAccessible* app = atspi_accessible_get_application(event->source, 0);
  if(!app) return;

  gchar* app_name = atspi_accessible_get_name(app, 0);
  AtspiRole role = atspi_accessible_get_role(event->source, 0);

  bool editable = is_editable(role);

  printf("name: %s role: %ld %s\n", app_name, (long)role, editable ? " editable" : "");

  if(app_name)
    g_free(app_name);
}

int main(){
  setlocale(LC_CTYPE, "C");
  if(!(listener = atspi_event_listener_new(on_event, 0, 0))){
    fprintf(stderr, "atspi_event_listener_new faield\n");
    return 1;
  }
  if(atspi_init()){
    fprintf(stderr, "atspi_init faield\n");
    return 1;
  }
  if(!atspi_event_listener_register(listener, "object:state-changed:focused", 0)){
    fprintf(stderr, "atspi_event_listener_register faield\n");
    return 1;
  }
  atspi_event_main();
  atspi_exit();
  return 0;
}
