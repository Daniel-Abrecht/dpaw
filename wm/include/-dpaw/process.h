#ifndef DPAW_PROCESS_H
#define DPAW_PROCESS_H

#include <sys/types.h>
#include <stdbool.h>
#include <-dpaw/primitives.h>
#include <-dpaw/linked_list.h>

typedef struct dpaw_process dpaw_process;
DPAW_DECLARE_CALLBACK_TYPE(dpaw_process)

struct dpaw_process {
  struct dpaw* dpaw;
  struct dpaw_list_entry dpaw_process_entry;
  struct dpaw_callback_list_dpaw_process exited;
  pid_t pid;
};

struct dpaw_process_create_options {
  const char*const* args;
  size_t fdmap_count;
  dpaw_int_pair_t* fdmap;
  dpaw_pcharc_pair_t* env;
  bool keep_env;
};

int dpaw_process_create_v(struct dpaw*, struct dpaw_process* process, const struct dpaw_process_create_options* options);
#define dpaw_process_create(DPAW, PROCESS, ...) \
  dpaw_process_create_v(DPAW, PROCESS, &(const struct dpaw_process_create_options){.args=__VA_ARGS__})
void dpaw_process_kill(struct dpaw_process* process, int signal);
void dpaw_process_cleanup(struct dpaw_process* process);

#endif
