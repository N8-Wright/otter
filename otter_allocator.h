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
