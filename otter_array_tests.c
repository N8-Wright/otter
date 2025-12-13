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
#include "otter_array.h"
#include "otter_test.h"

typedef struct {
  OTTER_ARRAY_DECLARE(int, integers);
} integer_list;

OTTER_TEST(array_init_length_is_zero) {
  integer_list list;
  OTTER_ARRAY_INIT(&list, integers, OTTER_TEST_ALLOCATOR);

  OTTER_ASSERT(OTTER_ARRAY_LENGTH(&list, integers) == 0);

  OTTER_TEST_END(otter_free(OTTER_TEST_ALLOCATOR, list.integers););
}

OTTER_TEST(array_init_capacity_is_greater_than_zero) {
  integer_list list;
  OTTER_ARRAY_INIT(&list, integers, OTTER_TEST_ALLOCATOR);

  OTTER_ASSERT(OTTER_ARRAY_CAPACITY(&list, integers) > 0);

  OTTER_TEST_END(otter_free(OTTER_TEST_ALLOCATOR, list.integers););
}

OTTER_TEST(array_init_non_null_allocation) {
  integer_list list;
  OTTER_ARRAY_INIT(&list, integers, OTTER_TEST_ALLOCATOR);

  OTTER_ASSERT(list.integers != NULL);

  OTTER_TEST_END(otter_free(OTTER_TEST_ALLOCATOR, list.integers););
}

static void *malloc_mock(otter_allocator *, size_t) { return NULL; }
OTTER_TEST(array_init_malloc_returns_null) {
  otter_allocator_vtable vtable = {
      .malloc = malloc_mock,
      .realloc = NULL,
      .free = NULL,
  };
  otter_allocator allocator = {
      .vtable = &vtable,
  };

  integer_list list;
  OTTER_ARRAY_INIT(&list, integers, &allocator);

  OTTER_ASSERT(list.integers == NULL);

  OTTER_TEST_END();
}

OTTER_TEST(array_append_resizes_automatically) {
  integer_list list;
  OTTER_ARRAY_INIT(&list, integers, OTTER_TEST_ALLOCATOR);

  for (int i = 0; i < 345; i++) {
    OTTER_ARRAY_APPEND(&list, integers, OTTER_TEST_ALLOCATOR, i);
    OTTER_ASSERT(OTTER_ARRAY_LENGTH(&list, integers) == (size_t)(i + 1));
    OTTER_ASSERT(OTTER_ARRAY_CAPACITY(&list, integers) >= (size_t)(i + 1));
  }

  for (size_t i = 0; i < 345; i++) {
    OTTER_ASSERT((int)i == OTTER_ARRAY_AT(&list, integers, i));
  }

  OTTER_TEST_END(otter_free(OTTER_TEST_ALLOCATOR, list.integers););
}

static void *realloc_mock(otter_allocator *, void *, size_t) { return NULL; }
OTTER_TEST(array_append_realloc_returns_null) {
  otter_allocator_vtable vtable = {
      .malloc = OTTER_TEST_ALLOCATOR->vtable->malloc,
      .realloc = realloc_mock,
      .free = NULL,
  };
  otter_allocator allocator = {
      .vtable = &vtable,
  };

  integer_list list;
  OTTER_ARRAY_INIT(&list, integers, &allocator);
  bool append_failed = false;
  for (int i = 0; i < 1000; i++) {
    if (!OTTER_ARRAY_APPEND(&list, integers, &allocator, i)) {
      append_failed = true;
      break;
    }
  }

  OTTER_ASSERT(append_failed);
  OTTER_TEST_END(otter_free(OTTER_TEST_ALLOCATOR, list.integers););
}
