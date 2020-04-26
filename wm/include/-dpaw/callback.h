#ifndef DPAW_CALLBACK_H
#define DPAW_CALLBACK_H

#include <-dpaw/linked_list.h>

// This is for making lists of callbacks & calling them

#define DPAW_DECLARE_CALLBACK_TYPE(T) \
  struct dpaw_callback_list_ ## T { \
    struct dpaw_list list; \
  }; \
  struct dpaw_callback_ ## T { \
    struct dpaw_list_entry entry; \
    void(*callback)(T*, void*, void*); \
    void* regptr; \
  }; \
  typedef void (*dpaw_callback_func_ ## T)(T* self, void* ptr, void* arg); \
  void dpaw_call_back_ ## T(struct dpaw_callback_list_ ## T*); \
  void dpaw_callback_add_ ## T(struct dpaw_callback_list_ ## T*, dpaw_callback_func_ ## T, void*); \
  void dpaw_callback_remove_ ## T(struct dpaw_callback_list_ ## T*, dpaw_callback_func_ ## T, void*);

struct dpaw_callback_list {
  struct dpaw_list list;
};

struct dpaw_callback {
  struct dpaw_list_entry entry;
  void (*callback)(void*, void*, void*);
  void* regptr;
};

#define DPAW_CALL_BACK(T, p, m, ptr) \
  dpaw_call_back(((struct dpaw_callback_list*)(struct dpaw_callback_list_ ## T*){&((T*){(p)})->m}), ((T*){(p)}), (ptr))

/* This function removes each callback after calling it, and it is guaranteed to not access the list after calling the last callback. */
#define DPAW_CALL_BACK_AND_REMOVE(T, p, m, ptr) \
  dpaw_call_back_and_remove(((struct dpaw_callback_list*)(struct dpaw_callback_list_ ## T*){&((T*){(p)})->m}), ((T*){(p)}), (ptr))

#define DPAW_CALLBACK_ADD(T, p, m, callback) \
  dpaw_callback_add(((struct dpaw_callback_list*)(struct dpaw_callback_list_ ## T*){&((T*){(p)})->m}), (struct dpaw_callback*)(struct dpaw_callback_ ## T*){(callback)})

#define DPAW_CALLBACK_REMOVE(callback) \
  dpaw_callback_remove(&(callback)->entry)

void dpaw_call_back_and_remove(struct dpaw_callback_list* list, void* self, void* callptr);
void dpaw_call_back(struct dpaw_callback_list*, void* self, void* callptr);
void dpaw_callback_add(struct dpaw_callback_list*, struct dpaw_callback*);
void dpaw_callback_remove(struct dpaw_list_entry*);

#endif
