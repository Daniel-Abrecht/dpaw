#include <-dpaw/log.h>
#ifndef RELEASE
#include <backtrace.h>
#include <backtrace-supported.h>
#endif
#include <unistd.h>

static int default_logfile = 1;

void dpaw_print_trace(int fd){
#ifdef RELEASE
  (void)fd;
  (void)default_logfile;
#else
  if(fd < 0)
    fd = default_logfile;
  FILE* logfile = fdopen(dup(fd),"a");
  struct backtrace_state *state = backtrace_create_state(0, BACKTRACE_SUPPORTS_THREADS, 0, 0);
  backtrace_print(state, 0, logfile);
  fclose(logfile);
#endif
}
