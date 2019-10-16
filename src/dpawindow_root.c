#include <stdio.h>
#include <dpawindow_root.h>

DEFINE_DPAWIN_DERIVED_WINDOW(root)


EV_ON(root, CreateNotify){
  (void)event;
  (void)window;
  puts("CreateNotify");
  return EHR_OK;
}

EV_ON(root, DestroyNotify){
  (void)event;
  (void)window;
  puts("DestroyNotify");
  return EHR_OK;
}

EV_ON(root, ReparentNotify){
  (void)event;
  (void)window;
  puts("ReparentNotify");
  return EHR_OK;
}
