#include <dpawin.h>
#include <stdlib.h>

static void cleanup(void){
  dpawin_cleanup();
}

int main(){
  if(dpawin_init() == -1)
    return 1;
  atexit(cleanup);
  if(dpawin_run() == -1)
    return 1;
  return 0;
}
