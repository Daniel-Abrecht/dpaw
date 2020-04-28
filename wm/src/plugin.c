#define _GNU_SOURCE
#include <sys/types.h>
#include <-dpaw/dpaw.h>
#include <-dpaw/plugin.h>
#include <-dpaw/dpaw_macros.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <dlfcn.h>

#define DPAW_PLUGIN_C_GENCODE

void dpaw_plugin_load_all(struct dpaw* dpaw){
  const char* plugin_path = getenv("DPAW_PLUGIN_PATH");
  if(!plugin_path)
    plugin_path = DPAW_PLUGIN_PATH;
  int dirfd = open(plugin_path, O_RDONLY | O_CLOEXEC | O_DIRECTORY);
  if(dirfd < 0){
    perror("open failed");
    return;
  }
  // There is no dlopen variant taking a file descriptor or at least the directory the file is in...
  static char buf[4096];
  ssize_t base_len = snprintf(buf, sizeof(buf), "/proc/self/fd/%d/", dirfd);
  if(base_len < 0 || (size_t)base_len >= sizeof(buf)-1){
    perror("snprintf failed");
    close(dirfd);
    return;
  }
  DIR* dir = fdopendir(dirfd);
  if(!dir){
    perror("fdopendir failed");
    close(dirfd);
    return;
  }
  for(struct dirent* entry; (entry=readdir(dir));){
    if(!(entry->d_type & DT_REG))
      continue;
    size_t namelen = strlen(entry->d_name);
    if(namelen < 3 || strcmp(&entry->d_name[namelen-3], ".so") || sizeof(buf)-(size_t)base_len <= namelen+1){
      fprintf(stderr, "Name of plugin too long: %s\n", entry->d_name);
      continue;
    }
    memcpy(buf+base_len, entry->d_name, namelen+1);
    dpaw_plugin_load(dpaw, buf);
  }
  closedir(dir);
}

void dpaw_plugin_unload_all(struct dpaw* dpaw){
  while(dpaw->plugin_list.first)
    dpaw_plugin_unload(container_of(dpaw->plugin_list.first, struct dpaw_plugin_private, dpaw_plugin_entry));
}

void dpaw_plugin_unload(struct dpaw_plugin_private* plugin){
  if(!plugin->dpaw){
    free(plugin);
    return;
  }
  dpaw_linked_list_set(0, &plugin->dpaw_plugin_entry, 0);
  if(plugin->initialised){
    if(plugin->plugin_cleanup)
      plugin->plugin_cleanup(&plugin->plugin);
  }
  plugin->initialised = false;
  if(plugin->lib)
    dlclose(plugin->lib);
  free(plugin);
}

struct dpaw_plugin_private* dpaw_plugin_load(struct dpaw* dpaw, const char* file){
  printf("Trying to load plugin: %s\n", file);
  struct dpaw_plugin_private* plugin = calloc(1,sizeof(struct dpaw_plugin_private));
  if(!plugin){
    perror("malloc failed");
    return 0;
  }

  plugin->dpaw = dpaw;

  plugin->lib = dlmopen(LM_ID_NEWLM, file, RTLD_NOW | RTLD_LOCAL | RTLD_DEEPBIND);
  if(!plugin->lib){
    puts("dlmopen failed");
    goto error;
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

  plugin->initialised = true;
  dpaw_linked_list_set(&dpaw->plugin_list, &plugin->dpaw_plugin_entry, 0);

  printf("Plugin loaded: %s\n", file);
  return plugin;

error:
  dpaw_plugin_unload(plugin);
  return 0;
}
