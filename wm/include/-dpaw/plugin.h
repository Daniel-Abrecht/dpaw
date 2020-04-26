#ifndef DPAW_PLUGIN_H
#define DPAW_PLUGIN_H

#include <api/dpaw/plugin.h>
#include <api/dpaw/plugin/all.h>

struct dpaw_plugin_private {
  struct dpaw_plugin plugin;
  struct dpaw* dpaw;
  void* lib;

#define FUNCTION_CODE_GENERATOR(R, N, P) \
  DPAW_P_CONCAT(DPAW_P_CONCAT(dpaw_, DPAW_PLUGIN_TYPE), _ ## N ## _t)* DPAW_P_CONCAT(DPAW_PLUGIN_TYPE, _ ## N);
#define DPAW_PLUGIN_C_GENCODE
#include <api/dpaw/plugin/all.h>
#undef DPAW_PLUGIN_C_GENCODE
#undef FUNCTION_CODE_GENERATOR
};

#endif
