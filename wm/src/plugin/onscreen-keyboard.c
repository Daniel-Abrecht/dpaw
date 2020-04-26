#include <api/dpaw/plugin/onscreen-keyboard.h>
#include <stdio.h>

int dpaw_onscreen_keyboard_input_ready(struct dpaw_plugin* plugin, bool ready, long focusee){
  (void)plugin;
  (void)ready;
  (void)focusee;
  puts("dpaw_onscreen_keyboard_input_ready: TODO\n");
  return 0;
}
