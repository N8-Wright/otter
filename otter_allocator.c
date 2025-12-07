/*
  otter Copyright (C) 2025 Nathaniel Wright

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "otter_allocator.h"
#include <stdlib.h>
static void otter_free_allocator_impl(otter_allocator *allocator) {
  free(allocator);
}

static void *otter_malloc_impl(otter_allocator *, size_t size) {
  return malloc(size);
}

static void *otter_realloc_impl(otter_allocator *, void *ptr, size_t size) {
  return realloc(ptr, size);
}

static void otter_free_impl(otter_allocator *, void *ptr) { free(ptr); }

static otter_allocator_vtable vtable = {
    .free_allocator = &otter_free_allocator_impl,
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
