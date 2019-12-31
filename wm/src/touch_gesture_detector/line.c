#include <dpaw/touch_gesture_detector/line.h>
#include <string.h>
#include <stdio.h>

static enum event_handler_result ontouch(
  struct dpaw_touch_gesture_detector* detector,
  struct dpaw_touch_event* te
){
  struct dpaw_line_touch_detector* lmd = container_of(detector, struct dpaw_line_touch_detector, detector);

  if(te->event.evtype != XI_TouchBegin)
    return EHR_UNHANDLED;

  struct dpaw_point touchpos = {
    .x = te->event.root_x,
    .y = te->event.root_y
  };

  struct dpaw_point P = dpaw_closest_point_on_line(lmd->params.line, touchpos, !lmd->params.noclip);
  const struct dpaw_point distance = dpaw_calc_distance(lmd->dpaw, P, touchpos, DPAW_UNIT_MICROMETER);
  if(distance.x * distance.x + distance.y * distance.y < 2500 * 2500)
    return EHR_OK;

  return EHR_UNHANDLED;
}

static const struct dpaw_touch_gesture_detector_type line_touch_detector = {
  .ontouch = ontouch
};

int dpaw_line_touch_init(
  struct dpaw_line_touch_detector* lmd,
  const struct dpaw_line_touch_detector_params* params,
  const struct dpaw* dpaw
){
  memset(lmd, 0, sizeof(*lmd));
  lmd->detector.type = &line_touch_detector;
  lmd->params = *params;
  lmd->dpaw = dpaw;
  return 0;
}

