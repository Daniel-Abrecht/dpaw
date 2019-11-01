#include <xev/X.c>
#include <xev/xinput2.c>
#include <dpawindow/workspace/handheld.h>
#include <dpawindow/root.h>
#include <dpawindow/app.h>
#include <dpawindow.h>
#include <dpawin.h>
#include <stdlib.h>
#include <stdio.h>

static struct dpawindow_handheld_window* lookup_xwindow(struct dpawindow_workspace_handheld* handheld_workspace, Window xwindow){
  struct dpawindow_app* app_window = dpawin_workspace_lookup_xwindow(&handheld_workspace->workspace, xwindow);
  if(!app_window || !app_window->workspace_private)
    return 0;
  return app_window->workspace_private;
}

static struct dpawin_rect determine_window_position(struct dpawindow_handheld_window* child){
  struct dpawin_rect boundary = {
    .top_left = {0,0}
  };
  struct dpawin_rect workspace_boundary = child->workspace->window.boundary;
  struct dpawin_point wh = {
    .x = workspace_boundary.bottom_right.x - workspace_boundary.top_left.x,
    .y = workspace_boundary.bottom_right.y - workspace_boundary.top_left.y
  };
  boundary.bottom_right.x = boundary.top_left.x + wh.x;
  boundary.bottom_right.y = boundary.top_left.y + wh.y;
  return boundary;
}

static int update_window_area(struct dpawindow_handheld_window* child){
  struct dpawin_rect boundary = determine_window_position(child);
  return dpawindow_place_window(&child->app_window->window, boundary);
}

static int make_current(struct dpawindow_handheld_window* child){
  printf("make_current %p -> %p\n", (void*)child->workspace->current, (void*)child);
  if(child->workspace->current == child)
    return 0;
  if(child->workspace->current)
    if(dpawindow_hide(&child->workspace->current->app_window->window, true))
      return -1;
  child->workspace->current = child;
  if(child){
    if(update_window_area(child))
      return -1;
    if(dpawindow_hide(&child->app_window->window, false))
      return -1;
  }
  return 0;
}


static int init(struct dpawindow_workspace_handheld* workspace){
  puts("handheld_workspace init");
  workspace->sideswipe.touchid = -1;
  workspace->sideswipe.mask = (1<<DPAWIN_DIRECTION_LEFT_TO_RIGHT) | (1<<DPAWIN_DIRECTION_RIGHT_TO_LEFT);
  return 0;
}

static void cleanup(struct dpawindow_workspace_handheld* workspace){
  (void)workspace;
  puts("handheld_workspace cleanup");
}

static int screen_added(struct dpawindow_workspace_handheld* workspace, struct dpawin_workspace_screen* screen){
  (void)workspace;
  (void)screen;
  puts("handheld_workspace screen_added");
  for(struct dpawindow_app* it = workspace->workspace.first_window; it; it++)
    update_window_area(it->workspace_private);
  return 0;
}

static int screen_changed(struct dpawindow_workspace_handheld* workspace, struct dpawin_workspace_screen* screen){
  (void)workspace;
  (void)screen;
  puts("handheld_workspace screen_changed");
  return 0;
}

static void screen_removed(struct dpawindow_workspace_handheld* workspace, struct dpawin_workspace_screen* screen){
  (void)workspace;
  (void)screen;
  puts("handheld_workspace screen_removed");
}

static int screen_make_bid(struct dpawindow_workspace_handheld* workspace, struct dpawin_workspace_screen* screen){
  (void)workspace;
  (void)screen;
  puts("handheld_workspace screen_make_bid");
  return 0;
}

static int take_window(struct dpawindow_workspace_handheld* workspace, struct dpawindow_app* window){
  struct dpawindow_handheld_window* child = calloc(sizeof(struct dpawindow_handheld_window), 1);
  if(!child)
    return -1;
  child->app_window = window;
  child->workspace = workspace;
  window->workspace_private = child;
  XReparentWindow(workspace->window.dpawin->root.display, window->window.xwindow, workspace->window.xwindow, 0, 0);
  if(make_current(child))
    return -1;
  dpawindow_set_mapping(&child->app_window->window, true);
  return 0;
}

static int abandon_window(struct dpawindow_app* window){
  if(window->next)
    make_current(window->next->workspace_private);
  return 0;
}

EV_ON(workspace_handheld, ConfigureRequest){
  struct dpawindow_handheld_window* child = lookup_xwindow(window, event->window);
  if(!child)
    return EHR_ERROR;
  struct dpawin_rect boundary = determine_window_position(child);
  XWindowChanges changes = {
    .x = boundary.top_left.x,
    .y = boundary.top_left.y,
    .width  = boundary.bottom_right.x - boundary.top_left.x,
    .height = boundary.bottom_right.y - boundary.top_left.y,
    .border_width = event->border_width,
    .sibling      = event->above,
    .stack_mode   = event->detail
  };
  XConfigureWindow(window->window.dpawin->root.display, event->window, event->value_mask | CWX | CWY | CWWidth | CWHeight, &changes);
  return EHR_OK;
}

EV_ON(workspace_handheld, XI_TouchBegin){
  puts("TouchBegin");
  if(!window->current)
    return EHR_UNHANDLED;
  if(window->sideswipe.touchid != -1)
    return EHR_UNHANDLED;
  const long boundary[] = {
    window->workspace.window->boundary.top_left.x,
    window->workspace.window->boundary.top_left.y,
    window->workspace.window->boundary.bottom_right.x,
    window->workspace.window->boundary.bottom_right.y
  };
  for(enum dpawin_direction direction=0; direction<4; direction++){
    if(!(window->sideswipe.mask & (1<<direction)))
      continue;
    long point = (direction % 2 ? event->root_y : event->root_x) + 0.5;
    long bound = boundary[direction];
    long distance = direction < 2 ? point - bound : bound - point;
    if(distance < 10 && distance >= -5){
      window->sideswipe.direction = direction;
      window->sideswipe.touchid = event->detail;
      window->sideswipe.confirmed = false;
      window->sideswipe.initial_position.x = event->root_x;
      window->sideswipe.initial_position.y = event->root_y;
      return EHR_NEXT;
    }
  }
  return EHR_UNHANDLED;
}

EV_ON(workspace_handheld, XI_TouchUpdate){
  (void)event;
  puts("TouchUpdate");
  if(!window->current){
    window->sideswipe.touchid = -1;
    window->sideswipe.match = false;
    window->sideswipe.last = 0;
    return EHR_UNHANDLED;
  }
  // TODO: Convert everything to physical units
  const long boundary[] = {
    window->workspace.window->boundary.top_left.x,
    window->workspace.window->boundary.top_left.y,
    window->workspace.window->boundary.bottom_right.x,
    window->workspace.window->boundary.bottom_right.y
  };
  enum dpawin_direction direction = window->sideswipe.direction;
  long switch_distance = 150; //window->sideswipe.switch_distance;
  long point = (direction % 2 ? event->root_y : event->root_x) + 0.5;
  long bound = boundary[direction];
  long distance = direction < 2 ? point - bound : bound - point;
  // Note: The since negative numbers are rounded down too, the previous
  long last = 0;
  if(window->sideswipe.match)
    last = window->sideswipe.last;
  long diff = (distance - last * switch_distance) / switch_distance;
  if(diff)
    window->sideswipe.last = (distance + (switch_distance/2)) / switch_distance;
  long offset = direction % 2 ? event->root_x - window->sideswipe.initial_position.x : event->root_y - window->sideswipe.initial_position.y;
#define MYABS(X) ((X)<0?-(X):(X))
  float ratio = distance ? (float)MYABS(offset) / MYABS(distance) : 0.0f;
#undef MYABS
  printf("point(%ld) bound(%ld) distance(%ld) last(%ld) diff(%ld) offset(%ld) ratio(%f)\n", point, bound, distance, last, diff, offset, ratio);
  bool firstmatch = false;
  if(!window->sideswipe.match){
    bool reasonable_distance = offset*offset + distance*distance > 15*15;
    if(distance < -5 || diff < 0 || (reasonable_distance && ratio > 0.5)){
      window->sideswipe.touchid = -1;
      window->sideswipe.match = false;
      window->sideswipe.last = 0;
      return EHR_UNHANDLED;
    }
    if(!diff && reasonable_distance)
      diff = distance < 0 ? -1 : 1;
    if(diff){
      window->sideswipe.match = true;
      window->sideswipe.last += 1;
      firstmatch = true;
    }
  }
  if(window->current && diff){
    struct dpawindow_app* appwin = window->current->app_window;
    if(diff > 0){
      while(diff-- > 0)
        appwin = appwin->next ? appwin->next : window->workspace.first_window;
    }else{
      while(diff++ < 0)
        appwin = appwin->previous ? appwin->previous : window->workspace.last_window;
    }
    make_current(appwin->workspace_private);
  }
  return firstmatch ? EHR_OK : EHR_NEXT;
}

EV_ON(workspace_handheld, XI_TouchEnd){
  puts("TouchEnd");
  if(window->sideswipe.touchid == event->detail){
    window->sideswipe.touchid = -1;
    window->sideswipe.match = false;
    window->sideswipe.last = 0;
    return EHR_OK;
  }
  return EHR_UNHANDLED;
}

DEFINE_DPAWIN_WORKSPACE( handheld,
  .init = init,
  .cleanup = cleanup,
  .take_window = take_window,
  .abandon_window = abandon_window,
  .screen_make_bid = screen_make_bid,
  .screen_added = screen_added,
  .screen_changed = screen_changed,
  .screen_removed = screen_removed
)
