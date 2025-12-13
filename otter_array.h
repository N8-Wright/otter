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
#ifndef OTTER_ARRAY_H_
#define OTTER_ARRAY_H_
#include "otter_allocator.h"
#include <stddef.h>

#define OTTER_ARRAY_LENGTH(arr, field) (arr)->field##_length
#define OTTER_ARRAY_CAPACITY(arr, field) (arr)->field##_capacity
#define OTTER_ARRAY_AT(arr, field, i) (arr)->field[i]

#define OTTER_ARRAY_DECLARE(type, field)                                       \
  size_t field##_length;                                                       \
  size_t field##_capacity;                                                     \
  type *field

#define OTTER_ARRAY_INIT(arr, field, allocator)                                \
  do {                                                                         \
    OTTER_ARRAY_LENGTH(arr, field) = 0;                                        \
    OTTER_ARRAY_CAPACITY(arr, field) = 5;                                      \
    (arr)->field = otter_malloc(                                               \
        allocator, sizeof(*(arr)->field) * OTTER_ARRAY_CAPACITY(arr, field));  \
  } while (0)

#define OTTER_ARRAY_EXPAND(arr, allocator, field, expanded)                    \
  do {                                                                         \
    size_t new_capacity = OTTER_ARRAY_CAPACITY(arr, field) * 2;                \
    void *result = otter_realloc(allocator, (arr)->field,                      \
                                 sizeof(*(arr)->field) * new_capacity);        \
    if (result != NULL) {                                                      \
      (arr)->field = result;                                                   \
      OTTER_ARRAY_CAPACITY(arr, field) = new_capacity;                         \
      expanded = true;                                                         \
    } else {                                                                   \
      expanded = false;                                                        \
    }                                                                          \
  } while (0)

#define OTTER_ARRAY_APPEND(arr, field, allocator, value)                       \
  ({                                                                           \
    bool should_append = true;                                                 \
    if (OTTER_ARRAY_LENGTH(arr, field) >= OTTER_ARRAY_CAPACITY(arr, field)) {  \
      OTTER_ARRAY_EXPAND(arr, allocator, field, should_append);                \
    }                                                                          \
    if (should_append) {                                                       \
      (arr)->field[OTTER_ARRAY_LENGTH(arr, field)++] = value;                  \
    }                                                                          \
    should_append;                                                             \
  })

#endif /* OTTER_ARRAY_H_ */
