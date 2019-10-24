#include <dpawin.h>
#include <stdlib.h>

static struct dpawin dpawin;

static void cleanup(void){
  dpawin_cleanup(&dpawin);
}

int main(){
  if(dpawin_init(&dpawin) == -1)
    return 1;
  atexit(cleanup);
  if(dpawin_run(&dpawin) == -1)
    return 1;
  return 0;
}
