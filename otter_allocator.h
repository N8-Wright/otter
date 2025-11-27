#ifndef OTTER_ALLOCATOR_H_
#define OTTER_ALLOCATOR_H_
#include <stddef.h>

typedef struct otter_allocator otter_allocator;
typedef struct otter_allocator_vtable {
  void *(*malloc)(struct otter_allocator *, size_t size);
  void *(*realloc)(struct otter_allocator *, void *ptr, size_t size);
  void (*free)(struct otter_allocator *, void *);
} otter_allocator_vtable;

typedef struct otter_allocator {
  otter_allocator_vtable *vtable;
} otter_allocator;

otter_allocator *otter_allocator_create();
void otter_allocator_free(otter_allocator *allocator);
void *otter_malloc(otter_allocator *, size_t size);
void *otter_realloc(otter_allocator *, void *ptr, size_t size);
void otter_free(otter_allocator *, void *ptr);

#endif /* OTTER_ALLOCATOR_H_ */
