#include <string.h>
#include <touch_gesture_detector/sideswipe.h>

static void reset(struct dpaw_touch_gesture_detector* detector){
  struct dpaw_sideswipe_detector* sideswipe = container_of(detector, struct dpaw_sideswipe_detector, detector);
  sideswipe->touchid = -1;
}

static enum event_handler_result ontouch(
  struct dpaw_touch_gesture_detector* detector,
  XIDeviceEvent* event,
  struct dpaw_rect bounds
){
  struct dpaw_sideswipe_detector* sideswipe = container_of(detector, struct dpaw_sideswipe_detector, detector);

  const long boundary[] = {
    bounds.top_left.x,
    bounds.top_left.y,
    bounds.bottom_right.x,
    bounds.bottom_right.y
  };

  switch(event->evtype){

    case XI_TouchBegin: {
      if(sideswipe->touchid != -1)
        return EHR_UNHANDLED;
      for(enum dpaw_direction direction=0; direction<4; direction++){
        if(!(sideswipe->params.mask & (1<<direction)))
          continue;
        long point = (direction % 2 ? event->root_y : event->root_x) + 0.5;
        long bound = boundary[direction];
        long distance = direction < 2 ? point - bound : bound - point;
        if(distance < 10 && distance >= -5){
          sideswipe->direction = direction;
          sideswipe->touchid = event->detail;
          sideswipe->match = false;
          sideswipe->last = 0;
          sideswipe->confirmed = false;
          sideswipe->initial_position.x = event->root_x;
          sideswipe->initial_position.y = event->root_y;
          return EHR_NEXT;
        }
      }
    } return EHR_UNHANDLED;

    case XI_TouchUpdate: {
      if(sideswipe->touchid != event->detail)
        return EHR_UNHANDLED;
      // TODO: Convert everything to physical units
      enum dpaw_direction direction = sideswipe->direction;
      long switch_distance = 150; //sideswipe->switch_distance;
      long point = (direction % 2 ? event->root_y : event->root_x) + 0.5;
      long bound = boundary[direction];
      long distance = direction < 2 ? point - bound : bound - point;
      // Note: The since negative numbers are rounded down too, the previous
      long last = 0;
      if(sideswipe->match)
        last = sideswipe->last;
      long diff = (distance - last * switch_distance) / switch_distance;
      if(diff)
        sideswipe->last = (distance + (switch_distance/2)) / switch_distance;
      long offset = direction % 2 ? event->root_x - sideswipe->initial_position.x : event->root_y - sideswipe->initial_position.y;
    #define MYABS(X) ((X)<0?-(X):(X))
      float ratio = distance ? (float)MYABS(offset) / MYABS(distance) : 0.0f;
    #undef MYABS
//      printf("point(%ld) bound(%ld) distance(%ld) last(%ld) diff(%ld) offset(%ld) ratio(%f)\n", point, bound, distance, last, diff, offset, ratio);
      bool firstmatch = false;
      if(!sideswipe->match){
        bool reasonable_distance = offset*offset + distance*distance > 15*15;
        if(distance < -5 || diff < 0 || (reasonable_distance && ratio > 0.5)){
          reset(&sideswipe->detector);
          return EHR_UNHANDLED;
        }
        if(!diff && reasonable_distance)
          diff = distance < 0 ? -1 : 1;
        if(diff){
          sideswipe->match = true;
          sideswipe->last += 1;
          firstmatch = true;
        }
      }
      if(diff)
        sideswipe->params.onswipe(sideswipe->params.private, direction, diff);
      return firstmatch ? EHR_OK : EHR_NEXT;
    } return EHR_UNHANDLED;

    case XI_TouchEnd: {
      if(sideswipe->touchid != event->detail)
        return EHR_UNHANDLED;
      reset(&sideswipe->detector);
    } return EHR_OK;

  }

  return EHR_UNHANDLED;
}

static const struct dpaw_touch_gesture_detector_type sideswipe_detector = {
  .reset = reset,
  .ontouch = ontouch,
};

int dpaw_sideswipe_init(
  struct dpaw_sideswipe_detector* sideswipe,
  const struct dpaw_sideswipe_detector_params* params
){
  memset(sideswipe, 0, sizeof(*sideswipe));
  sideswipe->detector.type = &sideswipe_detector;
  sideswipe->params = *params;
  reset(&sideswipe->detector);
  return 0;
}

