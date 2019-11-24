#include <dpaw.h>
#include <stdlib.h>
#include <stdio.h>

static struct dpaw dpaw;

static void cleanup(void){
  dpaw_cleanup(&dpaw);
}

int main(){
  if(dpaw_init(&dpaw) == -1)
    return 1;
  atexit(cleanup);
  if(dpaw_run(&dpaw) == -1)
    return 1;
  return 0;
}
