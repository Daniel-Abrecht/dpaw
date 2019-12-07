#include <dpaw.h>
#include <stdlib.h>
#include <stdio.h>

static struct dpaw dpaw;

static void cleanup(void){
  dpaw_cleanup(&dpaw);
}

int main(){
  fflush(stdout);
  setlinebuf(stdout); // If redirected to a file, we'll lose output otherwise, even the line below won't work !?!?
  setlinebuf(stderr);
  puts("Starting dpaw!");
  if(dpaw_init(&dpaw) == -1)
    return 1;
  atexit(cleanup);
  if(dpaw_run(&dpaw) == -1)
    return 1;
  return 0;
}
