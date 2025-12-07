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
#include "otter_lexer.h"
#include "otter_test.h"

OTTER_TEST(lexer_create_null_allocator) {
  otter_lexer *lexer = otter_lexer_create(NULL, "");
  OTTER_ASSERT(lexer == NULL);
  OTTER_TEST_END();
}

OTTER_TEST(lexer_create_null_source) {
  otter_lexer *lexer = otter_lexer_create(OTTER_TEST_ALLOCATOR, NULL);
  OTTER_ASSERT(lexer == NULL);
  OTTER_TEST_END();
}

static void *null_malloc(otter_allocator *, size_t) { return NULL; }

OTTER_TEST(lexer_create_allocator_returns_null) {
  otter_allocator_vtable vtable = {
      .malloc = null_malloc,
      .realloc = NULL,
      .free = NULL,
  };

  otter_allocator allocator = {
      .vtable = &vtable,
  };

  otter_lexer *lexer = otter_lexer_create(&allocator, "source");
  OTTER_ASSERT(lexer == NULL);
  OTTER_TEST_END();
}

typedef struct otter_allocator_mock {
  otter_allocator base;
  otter_allocator *real_allocator;
  int times_called_malloc;
} otter_allocator_mock;
static void *null_malloc2(otter_allocator *allocator, size_t size) {
  otter_allocator_mock *mock = (otter_allocator_mock *)allocator;
  mock->times_called_malloc++;
  if (mock->times_called_malloc > 1) {
    return NULL;
  }

  return otter_malloc(mock->real_allocator, size);
}
OTTER_TEST(lexer_create_allocator_second_malloc_returns_null) {
  otter_allocator_vtable vtable = {
      .malloc = null_malloc2,
      .realloc = OTTER_TEST_ALLOCATOR->vtable->realloc,
      .free = OTTER_TEST_ALLOCATOR->vtable->free,
  };

  otter_allocator_mock allocator = {
      .base =
          (otter_allocator){
              .vtable = &vtable,
          },
      .real_allocator = OTTER_TEST_ALLOCATOR,
      .times_called_malloc = 0,
  };

  otter_lexer *lexer =
      otter_lexer_create((otter_allocator *)&allocator, "source");
  OTTER_ASSERT(lexer == NULL);
  OTTER_TEST_END();
}
