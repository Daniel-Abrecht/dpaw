#define _GNU_SOURCE
#include <-dpaw/dpaw.h>
#include <-dpaw/plugin.h>
#include <stdio.h>
#include <dlfcn.h>

#define DPAW_PLUGIN_C_GENCODE

void dpaw_load_plugins(struct dpaw* dpaw){
  (void)dpaw;
}


void dpaw_unload_plugin(struct dpaw_plugin_private* plugin){
  if(!plugin->dpaw)
    return;
  (void)plugin; // TODO
}

int dpaw_load_plugin(struct dpaw* dpaw, struct dpaw_plugin_private* plugin, const char* file){
  memset(plugin, 0, sizeof(*plugin));

  plugin->dpaw = dpaw;

  plugin->lib = dlmopen(LM_ID_NEWLM, file, RTLD_NOW | RTLD_LOCAL | RTLD_DEEPBIND);
  if(!plugin->lib){
    puts("dlmopen failed");
    return -1;
  }

#define CALLBACK_CODE_GENERATOR(R, N, P) \
  { \
    DPAW_P_CONCAT(DPAW_P_CONCAT(dpaw_, DPAW_PLUGIN_TYPE), _ ## N ## _t)** func_ref = dlsym(plugin->lib, DPAW_P_STR(DPAW_P_CONCAT(DPAW_P_CONCAT(dpaw_, DPAW_PLUGIN_TYPE), _ ## N))); \
    if(func_ref) *func_ref = (DPAW_P_CONCAT(DPAW_P_CONCAT(dpaw_, DPAW_PLUGIN_TYPE), _ ## N)); \
  }
#include <api/dpaw/plugin/all.h>
#undef CALLBACK_CODE_GENERATOR

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#define FUNCTION_CODE_GENERATOR(R, N, P) \
  plugin->DPAW_P_CONCAT(DPAW_PLUGIN_TYPE, _ ## N) = dlsym(plugin->lib, DPAW_P_STR(DPAW_P_CONCAT(DPAW_P_CONCAT(dpaw_, DPAW_PLUGIN_TYPE), _ ## N)));
#include <api/dpaw/plugin/all.h>
#undef FUNCTION_CODE_GENERATOR
#pragma GCC diagnostic pop

  if(plugin->plugin_init && plugin->plugin_init(&plugin->plugin))
    goto error;

  return 0;
error:
  dpaw_unload_plugin(plugin);
  return -1;
}
