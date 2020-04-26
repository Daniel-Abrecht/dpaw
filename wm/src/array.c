#include <-dpaw/array.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

int dpaw_array_init_generic(struct dpaw_array* array, size_t size, size_t block_size, bool prealloc){
  if(!block_size)
    block_size = 1;
  memset(array, 0, sizeof(*array));
  array->size = size;
  array->block_size = block_size;
  if(!prealloc)
    return 0;
  void* data = calloc(array->block_size, array->size);
  if(!data){
    perror("calloc failed");
    return -1;
  }
  array->data = data;
  array->allocated = 1;
  return 0;
}

int dpaw_array_add_generic(struct dpaw_array* array, const void* data){
  size_t block_size = array->block_size ? array->block_size : 1;
  if(array->count >= array->allocated){
    size_t new_allocated = (array->count + block_size) / block_size * block_size;
    assert(new_allocated > array->count);
    void* memory = realloc(array->data, new_allocated * array->size);
    if(!memory){
      perror("realloc failed");
      return -1;
    }
    array->data = memory;
    array->allocated = new_allocated;
  }
  memcpy(((char*)array->data) + (array->count * array->size), data, array->size);
  array->count += 1;
  return 0;
}

void dpaw_array_remove_generic(struct dpaw_array* array, size_t index, size_t count){
  if(!count)
    return;
  assert(index+count <= array->count);
  size_t memove_count = array->count - index - count;
  if(memove_count){
    memmove(
      ((char*)array->data) + (index * array->size),
      ((char*)array->data) + ((index+count) * array->size),
      memove_count
    );
  }
  array->count -= count;
}

int dpaw_array_gc_generic(struct dpaw_array* array){
  size_t block_size = array->block_size ? array->block_size : 1;
  size_t new_allocated = (array->count + block_size - 1) / block_size * block_size;
  if(!new_allocated)
    new_allocated = 1;
  if(new_allocated != array->allocated){
    void* memory = realloc(array->data, new_allocated * array->size);
    if(!memory){
      perror("realloc failed");
      return -1;
    }
    array->data = memory;
    array->allocated = new_allocated;
  }
  return 0;
}

void dpaw_array_free_generic(struct dpaw_array* array){
  array->count = 0;
  array->allocated = 0;
  free(array->data);
  array->data = 0;
}
