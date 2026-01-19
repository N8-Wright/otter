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
#include "allocator.h"
#include "inc.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#define OTTER_ARRAY_LENGTH(arr, field) (arr)->field##_length
#define OTTER_ARRAY_CAPACITY(arr, field) (arr)->field##_capacity
#define OTTER_ARRAY_AT_UNSAFE(arr, field, i) ((arr)->field[i])
#define OTTER_ARRAY_AT(arr, field, i)                                          \
  ({                                                                           \
    typeof(*(arr)->field) item = {};                                           \
    if ((size_t)(i) < OTTER_ARRAY_LENGTH(arr, field)) {                        \
      item = (arr)->field[i];                                                  \
    } else {                                                                   \
      fprintf(                                                                 \
          stderr,                                                              \
          "Illegal access of array at index %zd when array is of size %zd\n",  \
          (size_t)(i), OTTER_ARRAY_LENGTH(arr, field));                        \
      exit(EXIT_FAILURE);                                                      \
    }                                                                          \
    item;                                                                      \
  })

#define OTTER_ARRAY_DECLARE(type, field)                                       \
  size_t field##_length;                                                       \
  size_t field##_capacity;                                                     \
  /* NOLINTBEGIN(bugprone-macro-parentheses) */                                \
  type *field /* NOLINTEND(bugprone-macro-parentheses) */

#define OTTER_ARRAY_INIT(arr, field, allocator)                                \
  do {                                                                         \
    OTTER_ARRAY_LENGTH(arr, field) = 0;                                        \
    OTTER_ARRAY_CAPACITY(arr, field) = 5;                                      \
    (arr)->field = otter_malloc(                                               \
        allocator, sizeof(*(arr)->field) * OTTER_ARRAY_CAPACITY(arr, field));  \
  } while (0)

bool otter_array_expand(otter_allocator *allocator, void **items,
                        size_t items_size, size_t *items_capacity);
#define OTTER_ARRAY_APPEND(arr, field, allocator, value)                       \
  ({                                                                           \
    bool should_append = true;                                                 \
    if (OTTER_ARRAY_LENGTH(arr, field) >= OTTER_ARRAY_CAPACITY(arr, field)) {  \
      should_append = otter_array_expand(allocator, (void **)&(arr)->field,    \
                                         sizeof(*(arr)->field),                \
                                         &OTTER_ARRAY_CAPACITY(arr, field));   \
    }                                                                          \
    if (should_append) {                                                       \
      (arr)->field[OTTER_ARRAY_LENGTH(arr, field)++] = value;                  \
    }                                                                          \
    should_append;                                                             \
  })

#define OTTER_ARRAY_FOREACH(arr, field, fn, ...)                               \
  for (size_t OTTER_UNIQUE_VARNAME(i) = 0;                                     \
       OTTER_UNIQUE_VARNAME(i) < OTTER_ARRAY_LENGTH(arr, field);               \
       OTTER_UNIQUE_VARNAME(i)++) {                                            \
    fn(__VA_ARGS__ __VA_OPT__(, )                                              \
           OTTER_ARRAY_AT_UNSAFE(arr, field, OTTER_UNIQUE_VARNAME(i)));        \
  }                                                                            \
  struct otter_useless_struct_to_allow_trailing_semicolon

#endif /* OTTER_ARRAY_H_ */
