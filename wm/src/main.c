#include <-dpaw/dpaw.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

static struct dpaw dpaw;

static void cleanup(void){
  dpaw_cleanup(&dpaw);
}

int main(int argc, char* argv[]){
  (void)argc;
  fflush(stdout);
  setlinebuf(stdout); // If redirected to a file, we'll lose output otherwise, even the line below won't work !?!?
  setlinebuf(stderr);
  puts("Starting dpaw!");
  if(dpaw_init(&dpaw) == -1)
    return 1;
  atexit(cleanup);
  int ret = dpaw_run(&dpaw);
  if(ret == -1)
    return 1;
  if(ret == 1){
    dpaw_cleanup(&dpaw);
    execv(argv[0], argv);
    exit(1);
  }
  return 0;
}
