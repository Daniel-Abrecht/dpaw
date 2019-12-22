#include <string.h>
#include <stdio.h>
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

  struct dpaw_point touchpos = {
    .x = event->root_x,
    .y = event->root_y
  };

  switch(event->evtype){

    case XI_TouchBegin: {
      if(sideswipe->touchid != -1)
        return EHR_UNHANDLED;
      for(enum dpaw_direction direction=0; direction<4; direction++){
        if(!(sideswipe->params.mask & (1<<direction)))
          continue;
        const struct dpaw_point boundary_edge = direction < 2 ? bounds.top_left : bounds.bottom_right;
        const struct dpaw_point start_point = direction % 2 ? (struct dpaw_point){touchpos.x, boundary_edge.y} : (struct dpaw_point){boundary_edge.x, touchpos.y};
        const struct dpaw_point distance = dpaw_calc_distance(sideswipe->dpaw, start_point, touchpos, DPAW_UNIT_MICROMETER);
        printf("%ldx%ld %ldx%ld %ldx%ld %ldx%ld\n", start_point.x, start_point.y, touchpos.x, touchpos.y, (touchpos.x-start_point.x), (touchpos.y-start_point.y), distance.x, distance.y);
        long axis_distance_from_screen_edge = (direction % 2 ? distance.y : distance.x) * (direction<2?1:-1);
        if(axis_distance_from_screen_edge < 2000 && axis_distance_from_screen_edge >= -500){
          sideswipe->direction = direction;
          sideswipe->touchid = event->detail;
          sideswipe->match = false;
          sideswipe->last = 0;
          sideswipe->confirmed = false;
          sideswipe->initial_position = touchpos;
          return EHR_NEXT;
        }
      }
    } return EHR_UNHANDLED;

    case XI_TouchUpdate: {
      if(sideswipe->touchid != event->detail)
        return EHR_UNHANDLED;
      // TODO: Convert everything to physical units
      long distance, offset;
      {
        const struct dpaw_point diff = dpaw_calc_distance(sideswipe->dpaw, sideswipe->initial_position, touchpos, DPAW_UNIT_MICROMETER);
        if(sideswipe->direction % 2){
          distance = diff.y;
          offset   = diff.x;
        }else{
          distance = diff.x;
          offset   = diff.y;
        }
        if(sideswipe->direction >= 2){
          distance = -distance;
          offset   = -offset;
        }
      }
      long switch_distance = sideswipe->params.switch_distance;
      // Note: The since negative numbers are rounded down too, the previous
      long last = 0;
      if(sideswipe->match)
        last = sideswipe->last;
      long diff = (distance - last * switch_distance) / switch_distance;
      if(diff)
        sideswipe->last = (distance + (switch_distance/2)) / switch_distance;
      #define MYABS(X) ((X)<0?-(X):(X))
      float ratio = (float)MYABS(offset) / MYABS(distance);
      #undef MYABS
      printf("distance(%ld) last(%ld) diff(%ld) offset(%ld) ratio(%f)\n", distance, last, diff, offset, ratio);
      bool firstmatch = false;
      if(!sideswipe->match){
        bool reasonable_distance = offset*offset + distance*distance > 5000 * 5000;
        if(distance < -500 || diff < 0 || (reasonable_distance && ratio > 0.5)){
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
        sideswipe->params.onswipe(sideswipe->params.private, sideswipe->direction, diff, sideswipe->initial_position.y);
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
  const struct dpaw_sideswipe_detector_params* params,
  const struct dpaw* dpaw
){
  memset(sideswipe, 0, sizeof(*sideswipe));
  sideswipe->detector.type = &sideswipe_detector;
  sideswipe->params = *params;
  if(!sideswipe->params.switch_distance)
    sideswipe->params.switch_distance = 40000;
  sideswipe->dpaw = dpaw;
  reset(&sideswipe->detector);
  return 0;
}

