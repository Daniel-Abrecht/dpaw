#include <sys/types.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdnoreturn.h>
#include <dpaw/dpaw.h>
#include <dpaw/process.h>


static noreturn void dpaw_process_create_sub_child(
  const struct dpaw_process_create_options* options
){
  if(!options->keep_env){
    if(options->env){
      size_t count = 0;
      for(dpaw_pcharc_pair_t* it=options->env; *it; it++)
        if(!(*it)[1])
          count++;
      {
        char* values[count+1]; // Count may be zero, but zero length arrays are invalid, so let's add one
        size_t i = 0;
        for(dpaw_pcharc_pair_t* it=options->env; *it; it++){
          if(!(*it)[1]){
            char* value = getenv((*it)[0]);
            values[i++] = value ? strdup(value) : 0;
          }
        }
        clearenv();
        i = 0;
        for(dpaw_pcharc_pair_t* it=options->env; *it; it++){
          if(!(*it)[1]){
            setenv(getenv((*it)[0]), values[i], true);
            if(values[i])
              free(values[i]);
            i++;
          }
        }
      }
    }else{
      clearenv();
    }
  }
  for(dpaw_pcharc_pair_t* it=options->env; *it; it++){
    if((*it)[1]){
      setenv((*it)[0], (*it)[1], true);
    }else if(options->keep_env){
      unsetenv((*it)[0]);
    }
  }
  for(size_t i=0,n=options->fdmap_count; i<n; i++)
    dup2(options->fdmap[i][0], options->fdmap[i][1]);
  execvp(options->args[0], (char*const*)options->args);
  _Exit(1);
}


int dpaw_process_create_v(
  struct dpaw* dpaw,
  struct dpaw_process* process,
  const struct dpaw_process_create_options* options
){
  memset(process, 0, sizeof(*process));
  process->dpaw = dpaw;
  int pid = fork();
  if(pid == -1){
    perror("fork failed");
    return -1;
  }
  if(pid){
    process->pid = pid;
    dpaw_linked_list_set(&dpaw->process_list, &process->dpaw_process_entry, 0);
    return 0;
  }else{
    dpaw_process_create_sub_child(options);
  }
  abort();
}

void dpaw_process_kill(struct dpaw_process* process, int signal){
  if(process->pid > 0)
    kill(process->pid, signal);
}

void dpaw_process_cleanup(struct dpaw_process* process){
  dpaw_linked_list_clear(&process->exited.list);
  dpaw_linked_list_set(0, &process->dpaw_process_entry, 0);
  memset(process, 0, sizeof(*process));
}
