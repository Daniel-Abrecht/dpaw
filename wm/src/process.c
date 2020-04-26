#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdnoreturn.h>
#include <-dpaw/dpaw.h>
#include <-dpaw/process.h>


static noreturn void dpaw_process_create_sub_child(
  int retfd,
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
  int error = errno;

  while(write(retfd, (unsigned char[]){error}, 1) != -1 && errno == EINTR);

  fprintf(stderr, "Error: execvp(\"%s\", {\"%s\"", options->args[0], options->args[0]);
  if(options->args[0])
  for(const char *const* it=options->args+1; *it; it++)
    fprintf(stderr, ", \"%s\"", *it);
  fprintf(stderr, "}): %s\n", strerror(error));
  fflush(stderr);

  _Exit(1);
}


int dpaw_process_create_v(
  struct dpaw* dpaw,
  struct dpaw_process* process,
  const struct dpaw_process_create_options* options
){
  if(!options->args || !options->args[0]){
    fprintf(stderr, "dpaw_process_create_v: no program specified!!!\n");
    return -1;
  }
  memset(process, 0, sizeof(*process));
  process->dpaw = dpaw;
  int check_fd[2];
  if(pipe(check_fd)){
    perror("pipe failed");
    return -1;
  }
  if(fcntl(check_fd[1], F_SETFD, FD_CLOEXEC) == -1){
    perror("fcntl(check_fd[1], F_SETFD, FD_CLOEXEC) failed");
    close(check_fd[0]);
    close(check_fd[1]);
    return -1;
  }
  int pid = fork();
  if(pid == -1){
    perror("fork failed");
    close(check_fd[0]);
    close(check_fd[1]);
    return -1;
  }
  if(pid){
    process->pid = pid;
    dpaw_linked_list_set(&dpaw->process_list, &process->dpaw_process_entry, 0);
    close(check_fd[1]);
    unsigned char ret = 0;
    while(true){
      int r = read(check_fd[0], &ret, 1);
      if(r == 0) break;
      if(r == -1 && errno == EINTR)
        continue;
      if(r != 0)
        ret = -1;
      break;
    }
    close(check_fd[0]);
    return ret;
  }else{
    close(check_fd[0]);
    dpaw_process_create_sub_child(check_fd[1], options);
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
