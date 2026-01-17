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
#include <stdarg.h>
#include <stdbool.h>
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

OTTER_TEST(strdup_allocator_null) {
  const char *str = "Foobar";
  char *dup = otter_strdup(NULL, str);

  OTTER_ASSERT(dup == NULL);
  OTTER_TEST_END();
}

OTTER_TEST(strdup_str_null) {
  const char *str = NULL;
  char *dup = otter_strdup(OTTER_TEST_ALLOCATOR, str);

  OTTER_ASSERT(dup == NULL);
  OTTER_TEST_END();
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

static void *malloc_mock(otter_allocator * /* unused */, size_t /* unused */) {
  return NULL;
}

OTTER_TEST(strndup_malloc_returns_null) {
  otter_allocator_vtable vtable = {
      .malloc = malloc_mock,
      .realloc = NULL,
      .free = NULL,
  };
  otter_allocator allocator = {
      .vtable = &vtable,
  };

  const char *str = "Foobar";
  char *dup = otter_strndup(&allocator, str, strlen(str));

  OTTER_ASSERT(dup == NULL);
  OTTER_TEST_END();
}

OTTER_TEST(strndup_str_null) {
  const char *str = NULL;
  char *dup = otter_strndup(OTTER_TEST_ALLOCATOR, str, 0);

  OTTER_ASSERT(dup == NULL);
  OTTER_TEST_END();
}

OTTER_TEST(strndup_allocator_null) {
  const char *str = "foobar";
  char *dup = otter_strndup(NULL, str, strlen(str));

  OTTER_ASSERT(dup == NULL);
  OTTER_TEST_END();
}

OTTER_TEST(strndup_partial_copy) {
  const char *str = "Hello World";
  const size_t substring_length = 5;
  char *dup = otter_strndup(OTTER_TEST_ALLOCATOR, str, substring_length);

  OTTER_ASSERT(dup != NULL);
  OTTER_ASSERT(strcmp(dup, "Hello") == 0);
  OTTER_ASSERT(strlen(dup) == substring_length);

  OTTER_TEST_END(otter_free(OTTER_TEST_ALLOCATOR, dup););
}

OTTER_TEST(vasprintf_allocator_null) {
  char *str = NULL;
  va_list args;
  bool result = otter_vasprintf(NULL, &str, "test", args);

  OTTER_ASSERT(result == false);
  OTTER_ASSERT(str == NULL);
  OTTER_TEST_END();
}

OTTER_TEST(vasprintf_str_null) {
  va_list args;
  bool result = otter_vasprintf(OTTER_TEST_ALLOCATOR, NULL, "test", args);

  OTTER_ASSERT(result == false);
  OTTER_TEST_END();
}

OTTER_TEST(vasprintf_fmt_null) {
  char *str = NULL;
  va_list args;
  bool result = otter_vasprintf(OTTER_TEST_ALLOCATOR, &str, NULL, args);

  OTTER_ASSERT(result == false);
  OTTER_ASSERT(str == NULL);
  OTTER_TEST_END();
}

static bool vasprintf_helper(otter_allocator *allocator, char **str,
                             const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  bool result = otter_vasprintf(allocator, str, fmt, args);
  va_end(args);
  return result;
}

OTTER_TEST(vasprintf_works) {
  char *str = NULL;
  const int number = 42;
  bool result = vasprintf_helper(OTTER_TEST_ALLOCATOR, &str, "Hello %s %d",
                                 "World", number);

  OTTER_ASSERT(result == true);
  OTTER_ASSERT(str != NULL);
  OTTER_ASSERT(strcmp(str, "Hello World 42") == 0);

  OTTER_TEST_END(otter_free(OTTER_TEST_ALLOCATOR, str););
}

OTTER_TEST(vasprintf_malloc_fails) {
  otter_allocator_vtable vtable = {
      .malloc = malloc_mock,
      .realloc = NULL,
      .free = NULL,
  };
  otter_allocator allocator = {
      .vtable = &vtable,
  };

  char *str = NULL;
  const int number = 123;
  bool result = vasprintf_helper(&allocator, &str, "test %d", number);

  OTTER_ASSERT(result == false);
  OTTER_ASSERT(str == NULL);
  OTTER_TEST_END();
}

OTTER_TEST(asprintf_allocator_null) {
  char *str = NULL;
  bool result = otter_asprintf(NULL, &str, "test");

  OTTER_ASSERT(result == false);
  OTTER_ASSERT(str == NULL);
  OTTER_TEST_END();
}

OTTER_TEST(asprintf_str_null) {
  bool result = otter_asprintf(OTTER_TEST_ALLOCATOR, NULL, "test");

  OTTER_ASSERT(result == false);
  OTTER_TEST_END();
}

OTTER_TEST(asprintf_fmt_null) {
  char *str = NULL;
  bool result = otter_asprintf(OTTER_TEST_ALLOCATOR, &str, NULL);

  OTTER_ASSERT(result == false);
  OTTER_ASSERT(str == NULL);
  OTTER_TEST_END();
}

OTTER_TEST(asprintf_works) {
  char *str = NULL;
  const int number = 999;
  bool result =
      otter_asprintf(OTTER_TEST_ALLOCATOR, &str, "Number: %d", number);

  OTTER_ASSERT(result == true);
  OTTER_ASSERT(str != NULL);
  OTTER_ASSERT(strcmp(str, "Number: 999") == 0);

  OTTER_TEST_END(otter_free(OTTER_TEST_ALLOCATOR, str););
}

OTTER_TEST(asprintf_empty_format) {
  char *str = NULL;
  bool result = otter_asprintf(OTTER_TEST_ALLOCATOR, &str, "");

  OTTER_ASSERT(result == true);
  OTTER_ASSERT(str != NULL);
  OTTER_ASSERT(strcmp(str, "") == 0);

  OTTER_TEST_END(otter_free(OTTER_TEST_ALLOCATOR, str););
}
