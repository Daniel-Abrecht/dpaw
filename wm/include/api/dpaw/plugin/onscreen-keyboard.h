#if !defined(DPAW_API_PLUGIN_ONSCREEN_KEYBOARD_H) || defined(DPAW_PLUGIN_C_GENCODE)
#define DPAW_API_PLUGIN_ONSCREEN_KEYBOARD_H

#ifndef DPAW_PLUGIN_C_GENCODE
#include "../plugin.h"
#include <stdbool.h>
#endif

#define DPAW_PLUGIN_TYPE onscreen_keyboard
#define DPAW_PLUGIN_CALLBACKS(F) \
  F(int, input_ready, (struct dpaw_plugin*, bool ready, long focusee))

#include "../plugin.template"

#endif
