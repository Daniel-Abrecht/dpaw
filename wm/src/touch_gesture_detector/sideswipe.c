#include <-dpaw/touch_gesture_detector/sideswipe.h>
#include <string.h>

static void reset(struct dpaw_touch_gesture_detector* detector){
  struct dpaw_sideswipe_detector* sideswipe = container_of(detector, struct dpaw_sideswipe_detector, detector);
  sideswipe->touch_source = -1;
}

static enum event_handler_result ontouch(
  struct dpaw_touch_gesture_detector* detector,
  struct dpaw_touch_event* te
){
  struct dpaw_sideswipe_detector* sideswipe = container_of(detector, struct dpaw_sideswipe_detector, detector);

  if(!sideswipe->params.bounds)
    return EHR_UNHANDLED;

  struct dpaw_point touchpos = {
    .x = te->event.root_x,
    .y = te->event.root_y
  };

  switch(te->event.evtype){

    case XI_TouchBegin: {
      if(sideswipe->touch_source != -1)
        return EHR_UNHANDLED;
      for(enum dpaw_direction direction=0; direction<4; direction++){
        if(!(sideswipe->params.mask & (1<<direction)))
          continue;
        const struct dpaw_point boundary_edge = direction < 2 ? sideswipe->params.bounds->top_left : sideswipe->params.bounds->bottom_right;
        const struct dpaw_point start_point = direction % 2 ? (struct dpaw_point){touchpos.x, boundary_edge.y} : (struct dpaw_point){boundary_edge.x, touchpos.y};
        const struct dpaw_point distance = dpaw_calc_distance(sideswipe->dpaw, start_point, touchpos, DPAW_UNIT_MICROMETER);
        long axis_distance_from_screen_edge = (direction % 2 ? distance.y : distance.x) * (direction<2?1:-1);
        if(axis_distance_from_screen_edge < 2000 && axis_distance_from_screen_edge >= -500){
          sideswipe->direction = direction;
          sideswipe->touch_source = te->touch_source;
          sideswipe->initial_position = touchpos;
          return EHR_NEXT;
        }
      }
    } return EHR_UNHANDLED;

    case XI_TouchUpdate: {
      if(sideswipe->touch_source != te->touch_source)
        return EHR_UNHANDLED;
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
      #define MYABS(X) ((X)<0?-(X):(X))
      float ratio = (float)MYABS(offset) / MYABS(distance);
      #undef MYABS
      bool reasonable_distance = offset*offset + distance*distance > 5000 * 5000;
      if(distance < -500 || (reasonable_distance && ratio > 0.5)){
        reset(detector);
        return EHR_UNHANDLED;
      }
      if(!reasonable_distance)
        return EHR_NEXT;
      dpaw_gesture_detected(detector, 1, &te->touch_source);
    } return EHR_OK;

    case XI_TouchEnd: {
      if(sideswipe->touch_source != te->touch_source)
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
  const struct dpaw* dpaw,
  const struct dpaw_sideswipe_detector_params* params
){
  if(!dpaw)
    return -1;
  memset(sideswipe, 0, sizeof(*sideswipe));
  sideswipe->detector.type = &sideswipe_detector;
  if(params)
    sideswipe->params = *params;
  sideswipe->dpaw = dpaw;
  reset(&sideswipe->detector);
  return 0;
}

