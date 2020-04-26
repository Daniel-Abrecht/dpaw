#if !defined(DPAW_API_PLUGIN_PLUGIN_H) || defined(DPAW_PLUGIN_C_GENCODE)
#define DPAW_API_PLUGIN_PLUGIN_H

#ifndef DPAW_PLUGIN_C_GENCODE
#include "../plugin.h"
#endif

#define DPAW_PLUGIN_TYPE plugin
#define DPAW_PLUGIN_FUNCTIONS(F) \
  F(int, init, (struct dpaw_plugin*)) \
  F(void, cleanup, (struct dpaw_plugin*))

#include "../plugin.template"

#endif
