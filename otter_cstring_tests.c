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
#include "otter_cstring.h"
#include "otter_test.h"
#include <stddef.h>
#include <string.h>
OTTER_TEST(strdup_works) {
  const char *str = "Foobar";
  char *dup = otter_strdup(OTTER_TEST_ALLOCATOR, str);

  OTTER_ASSERT(dup != NULL);
  OTTER_ASSERT(str != dup);
  OTTER_ASSERT(strcmp(str, dup) == 0);

  OTTER_TEST_END(otter_free(OTTER_TEST_ALLOCATOR, dup););
}

OTTER_TEST(strdup_empty) {
  const char *str = "";
  char *dup = otter_strdup(OTTER_TEST_ALLOCATOR, str);

  OTTER_ASSERT(dup != NULL);
  OTTER_ASSERT(str != dup);
  OTTER_ASSERT(strcmp(str, dup) == 0);

  OTTER_TEST_END(otter_free(OTTER_TEST_ALLOCATOR, dup););
}

OTTER_TEST(strndup_zero_size) {
  const char *str = "Foobar";
  char *dup = otter_strndup(OTTER_TEST_ALLOCATOR, str, 0);
  OTTER_ASSERT(dup != NULL);
  OTTER_ASSERT(str != dup);
  OTTER_ASSERT(strcmp("", dup) == 0);
  OTTER_ASSERT(strcmp(str, dup) != 0);

  OTTER_TEST_END(otter_free(OTTER_TEST_ALLOCATOR, dup););
}
