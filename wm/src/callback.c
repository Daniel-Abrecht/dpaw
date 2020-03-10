#include <dpaw/callback.h>

void dpaw_call_back(struct dpaw_callback_list* list, void* self, void* callptr){
  for(struct dpaw_list_entry *it=list->list.first; it; it=it->next){
    struct dpaw_callback* callback = container_of(it, struct dpaw_callback, entry);
    callback->callback(self, callback->regptr, callptr);
  }
}

void dpaw_call_back_and_remove(struct dpaw_callback_list* list, void* self, void* callptr){
  struct dpaw_list_entry *it = list->list.first;
  while(it){
    struct dpaw_callback* callback = container_of(it, struct dpaw_callback, entry);
    it = it->next;
    dpaw_callback_remove(&callback->entry);
    callback->callback(self, callback->regptr, callptr);
  }
}

void dpaw_callback_add(struct dpaw_callback_list* list, struct dpaw_callback* callback){
  dpaw_linked_list_set(&list->list, &callback->entry, list->list.first);
}

void dpaw_callback_remove(struct dpaw_list_entry* callback){
  dpaw_linked_list_set(0, callback, 0);
}
