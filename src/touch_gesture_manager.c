#include <touch_gesture_manager.h>
#include <string.h>
#include <stdio.h>

int dpaw_touch_gesture_manager_init(struct dpaw_touch_gesture_manager* manager){
  memset(manager, 0, sizeof(*manager));
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

// TODO: Allow detecting multiple touchpoints at once.
enum event_handler_result dpaw_touch_gesture_manager_dispatch_touch(
  struct dpaw_touch_gesture_manager* manager,
  XIDeviceEvent* event,
  struct dpaw_rect bounds
){
  if(manager->detected){
    enum event_handler_result result = manager->detected->type->ontouch(manager->detected, event, bounds);
    if(result != EHR_NEXT && result != EHR_OK){
      if(manager->detected->type->reset)
        manager->detected->type->reset(manager->detected);
      manager->detected = 0;
      return result;
    }
    return EHR_OK;
  }
  bool all_unhandled = true;
  for(struct dpaw_list_entry* it = manager->detector_list.first; it; it=it->next){
    struct dpaw_touch_gesture_detector* detector = container_of(it, struct dpaw_touch_gesture_detector, manager_entry);
    if(detector->type->ontouch){
      enum event_handler_result result = detector->type->ontouch(detector, event, bounds);
      if(result == EHR_OK){
        manager->detected = detector;
        for(struct dpaw_list_entry* it2 = manager->detector_list.first; it2; it2=it2->next){
          if(it == it2)
            continue;
          struct dpaw_touch_gesture_detector* detector = container_of(it, struct dpaw_touch_gesture_detector, manager_entry);
          if(detector->type->reset)
            detector->type->reset(detector);
        }
        return result;
      }
      if(result == EHR_NEXT)
        all_unhandled = false;
    }
  }
  return all_unhandled ? EHR_UNHANDLED : EHR_NEXT;
}
