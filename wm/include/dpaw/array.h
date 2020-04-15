#ifndef DPAW_ARRAY_H
#define DPAW_ARRAY_H

#include <stddef.h>
#include <stdbool.h>

struct dpaw_array {
  void * data;
  const void* tmp;
  size_t size;
  size_t count;
  size_t block_size;
  size_t allocated;
};

#define DPAW_ARRAY(T) \
  union { \
    struct dpaw_array array; \
    struct { \
      T * data; \
      const T* tmp; /* Used for a type check. Not pretty, but works... */ \
      size_t size; \
      size_t count; \
      size_t block_size; \
      size_t allocated; \
    }; \
  }

int dpaw_array_init_generic(struct dpaw_array* array, size_t size, size_t block_size, bool prealloc);
#define dpaw_array_init(X, C, I) dpaw_array_init_generic(&(X)->array, sizeof(*(X)->data), (C), (I))
int dpaw_array_add_generic(struct dpaw_array* array, const void* data);
#define dpaw_array_add(X, V) dpaw_array_add_generic(&(X)->array, ((X)->tmp=(V), (V)))
void dpaw_array_remove_generic(struct dpaw_array* array, size_t index, size_t count);
#define dpaw_array_remove(X, I, N) dpaw_array_remove_generic(&(X)->array, (I), (N))
int dpaw_array_gc_generic(struct dpaw_array* array);
#define dpaw_array_gc(X) dpaw_array_gc_generic(&(X)->array)
void dpaw_array_free_generic(struct dpaw_array* array);
#define dpaw_array_free(X) dpaw_array_free_generic(&(X)->array)

#endif
