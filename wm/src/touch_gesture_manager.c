#include <dpaw/dpaw.h>
#include <dpaw/touch_gesture_manager.h>
#include <string.h>
#include <stdio.h>

int dpaw_touch_gesture_manager_init(struct dpaw_touch_gesture_manager* manager, struct dpaw* dpaw){
  memset(manager, 0, sizeof(*manager));
  manager->dpaw = dpaw;
  return 0;
}

void dpaw_touch_gesture_manager_reset(struct dpaw_touch_gesture_manager* manager){
  for(struct dpaw_list_entry* it = manager->detector_list.first; it; it=it->next){
    struct dpaw_touch_gesture_detector* detector = container_of(it, struct dpaw_touch_gesture_detector, manager_entry);
    if(detector->type->reset)
      detector->type->reset(detector);
  }
}

void dpaw_touch_gesture_manager_remove_detector(
  struct dpaw_touch_gesture_detector* detector
){
  if(!detector->manager_entry.list)
    return;
  dpaw_linked_list_set(0, &detector->manager_entry, 0);
  if(detector->type->cleanup)
    detector->type->cleanup(detector);
}

void dpaw_touch_gesture_manager_cleanup(struct dpaw_touch_gesture_manager* manager){
  for(struct dpaw_list_entry* it = manager->detector_list.first; it; it=it->next){
    struct dpaw_touch_gesture_detector* detector = container_of(it, struct dpaw_touch_gesture_detector, manager_entry);
    dpaw_touch_gesture_manager_remove_detector(detector);
  }
}

void dpaw_gesture_detected(struct dpaw_touch_gesture_detector* detector, unsigned touch_source_count, int touch_source_list[touch_source_count]){
  if(!detector->manager_entry.list) return;
  struct dpaw_touch_gesture_manager* manager = container_of(detector->manager_entry.list, struct dpaw_touch_gesture_manager, detector_list);
  struct dpaw* dpaw = manager->dpaw;
  if(detector->ongesture)
    detector->ongesture(detector->private, detector);
  for(unsigned i=0; i<touch_source_count; i++){
    int touch_source = touch_source_list[i];
    if(touch_source < 0 || touch_source >= DPAW_WORKSPACE_MAX_TOUCH_SOURCES)
      continue;
    struct dpaw_touchevent_window_map* ts = &dpaw->touch_source[touch_source];
    if(!ts->gesture_detector)
      ts->gesture_detector = detector;
  }
}

int dpaw_touch_gesture_manager_add_detector(
  struct dpaw_touch_gesture_manager* manager,
  struct dpaw_touch_gesture_detector* detector
){
  dpaw_touch_gesture_manager_remove_detector(detector);
  if(detector->type->reset)
    detector->type->reset(detector);
  dpaw_linked_list_set(&manager->detector_list, &detector->manager_entry, 0);
  return 0;
}

enum event_handler_result dpaw_touch_gesture_manager_dispatch_touch(
  struct dpaw_touch_gesture_manager* manager,
  struct dpaw_touch_event* event
){
  if(!event->twm)
    return EHR_UNHANDLED;
  if(event->twm->gesture_detector){
    enum event_handler_result result = event->twm->gesture_detector->ontouch ? event->twm->gesture_detector->ontouch(event->twm->gesture_detector->private, event->twm->gesture_detector, event) : EHR_NEXT;
    if(event->event.evtype == XI_TouchEnd)
      result = EHR_OK;
    if((result != EHR_NEXT && result != EHR_OK) || event->event.evtype == XI_TouchEnd){
      if(event->twm->gesture_detector->type->reset)
        event->twm->gesture_detector->type->reset(event->twm->gesture_detector);
      event->twm->gesture_detector = 0;
      return result;
    }
    return EHR_OK;
  }
  bool all_unhandled = true;
  for(struct dpaw_list_entry* it = manager->detector_list.first; it; it=it->next){
    struct dpaw_touch_gesture_detector* detector = container_of(it, struct dpaw_touch_gesture_detector, manager_entry);
    if(detector->type->ontouch){
      enum event_handler_result result = detector->type->ontouch(detector, event);
      if(result == EHR_OK){
        event->twm->gesture_detector = detector;
        return result;
      }
      if(result == EHR_NEXT)
        all_unhandled = false;
    }
  }
  return all_unhandled ? EHR_UNHANDLED : EHR_NEXT;
}
