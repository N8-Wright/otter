#include "otter_allocator.h"
#include <stdlib.h>
static void *otter_malloc_impl(otter_allocator *, size_t size) {
  return malloc(size);
}

static void *otter_realloc_impl(otter_allocator *, void *ptr, size_t size) {
  return realloc(ptr, size);
}

static void otter_free_impl(otter_allocator *, void *ptr) { free(ptr); }

static otter_allocator_vtable vtable = {
    .malloc = &otter_malloc_impl,
    .realloc = &otter_realloc_impl,
    .free = &otter_free_impl,
};

otter_allocator *otter_allocator_create() {
  otter_allocator *allocator = malloc(sizeof(*allocator));
  if (allocator == NULL) {
    return allocator;
  }

  allocator->vtable = &vtable;
  return allocator;
}

void otter_allocator_free(otter_allocator *allocator) { free(allocator); }

void *otter_malloc(otter_allocator *allocator, size_t size) {
  return allocator->vtable->malloc(allocator, size);
}

void *otter_realloc(otter_allocator *allocator, void *ptr, size_t size) {
  return allocator->vtable->realloc(allocator, ptr, size);
}

void otter_free(otter_allocator *allocator, void *ptr) {
  allocator->vtable->free(allocator, ptr);
}
