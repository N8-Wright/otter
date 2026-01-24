/*
  otter Copyright (C) 2026 Nathaniel Wright

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
#include "otter/string.h"
#include "otter/test.h"
#include <string.h>

#define LARGE_BUFFER_SIZE 1024

OTTER_TEST(string_create_basic) {
  const char *test_str = "hello";
  otter_string *str =
      otter_string_create(OTTER_TEST_ALLOCATOR, test_str, strlen(test_str));

  OTTER_ASSERT(str != NULL);
  OTTER_ASSERT(otter_string_length(str) == strlen(test_str));
  OTTER_ASSERT(strcmp(otter_string_cstr(str), test_str) == 0);

  OTTER_TEST_END(otter_string_free(str););
}

OTTER_TEST(string_create_empty) {
  const char *test_str = "";
  otter_string *str =
      otter_string_create(OTTER_TEST_ALLOCATOR, test_str, strlen(test_str));

  OTTER_ASSERT(str != NULL);
  OTTER_ASSERT(otter_string_length(str) == 0);
  OTTER_ASSERT(strcmp(otter_string_cstr(str), "") == 0);

  OTTER_TEST_END(otter_string_free(str););
}

OTTER_TEST(string_create_null_allocator) {
  const char *test_str = "hello";
  otter_string *str = otter_string_create(NULL, test_str, strlen(test_str));

  OTTER_ASSERT(str == NULL);

  OTTER_TEST_END();
}

OTTER_TEST(string_create_null_string) {
  const size_t dummy_length = 5;
  otter_string *str =
      otter_string_create(OTTER_TEST_ALLOCATOR, NULL, dummy_length);

  OTTER_ASSERT(str == NULL);

  OTTER_TEST_END();
}

OTTER_TEST(string_format_basic) {
  const int integer = 10;
  otter_string *str = otter_string_format(OTTER_TEST_ALLOCATOR, "%d", integer);
  OTTER_ASSERT(str != NULL);
  OTTER_ASSERT(otter_string_compare_cstr(str, "10") == 0);
  OTTER_TEST_END(otter_string_free(str););
}

OTTER_TEST(string_format_null_allocator) {
  const char *test_str = "hello";
  otter_string *str = otter_string_format(NULL, test_str);
  OTTER_ASSERT(str == NULL);
  OTTER_TEST_END(otter_string_free(str););
}

OTTER_TEST(string_format_null_string) {
  otter_string *str = otter_string_format(OTTER_TEST_ALLOCATOR, NULL);
  OTTER_ASSERT(str == NULL);
  OTTER_TEST_END(otter_string_free(str););
}

OTTER_TEST(string_format_invalid_format_str) {
  const char *arg = ", World!";
  const int extra_arg = 111;
  otter_string *str =
      otter_string_format(OTTER_TEST_ALLOCATOR, "Hello %s %", arg, extra_arg);
  OTTER_ASSERT(str == NULL);
  OTTER_TEST_END(otter_string_free(str););
}

OTTER_TEST(string_from_cstr) {
  const char *test_str = "hello world";
  otter_string *str = otter_string_from_cstr(OTTER_TEST_ALLOCATOR, test_str);

  OTTER_ASSERT(str != NULL);
  OTTER_ASSERT(otter_string_length(str) == strlen(test_str));
  OTTER_ASSERT(strcmp(otter_string_cstr(str), test_str) == 0);

  OTTER_TEST_END(otter_string_free(str););
}

OTTER_TEST(string_from_cstr_null) {
  otter_string *str = otter_string_from_cstr(OTTER_TEST_ALLOCATOR, NULL);

  OTTER_ASSERT(str == NULL);

  OTTER_TEST_END();
}

OTTER_TEST(string_append_basic) {
  const char *initial = "hello";
  const char *append_str = " world";
  otter_string *str = otter_string_from_cstr(OTTER_TEST_ALLOCATOR, initial);

  OTTER_ASSERT(str != NULL);
  otter_string_append(&str, append_str, strlen(append_str));

  OTTER_ASSERT(otter_string_length(str) ==
               strlen(initial) + strlen(append_str));
  OTTER_ASSERT(strcmp(otter_string_cstr(str), "hello world") == 0);

  OTTER_TEST_END(otter_string_free(str););
}

OTTER_TEST(string_append_empty) {
  const char *initial = "hello";
  const char *append_str = "";
  otter_string *str = otter_string_from_cstr(OTTER_TEST_ALLOCATOR, initial);

  OTTER_ASSERT(str != NULL);
  otter_string_append(&str, append_str, strlen(append_str));

  OTTER_ASSERT(otter_string_length(str) == strlen(initial));
  OTTER_ASSERT(strcmp(otter_string_cstr(str), "hello") == 0);

  OTTER_TEST_END(otter_string_free(str););
}

OTTER_TEST(string_append_to_empty) {
  const char *initial = "";
  const char *append_str = "world";
  otter_string *str = otter_string_from_cstr(OTTER_TEST_ALLOCATOR, initial);

  OTTER_ASSERT(str != NULL);
  otter_string_append(&str, append_str, strlen(append_str));

  OTTER_ASSERT(otter_string_length(str) == strlen(append_str));
  OTTER_ASSERT(strcmp(otter_string_cstr(str), "world") == 0);

  OTTER_TEST_END(otter_string_free(str););
}

OTTER_TEST(string_append_multiple) {
  otter_string *str = otter_string_from_cstr(OTTER_TEST_ALLOCATOR, "a");

  OTTER_ASSERT(str != NULL);

  const int append_count = 100;
  for (int i = 0; i < append_count; i++) {
    otter_string_append(&str, "b", 1);
  }

  const size_t expected_length = 101;
  OTTER_ASSERT(otter_string_length(str) == expected_length);
  OTTER_ASSERT(otter_string_cstr(str)[0] == 'a');
  for (size_t i = 1; i < expected_length; i++) {
    OTTER_ASSERT(otter_string_cstr(str)[i] == 'b');
  }

  OTTER_TEST_END(otter_string_free(str););
}

OTTER_TEST(string_append_null_pointer) {
  otter_string_append(NULL, "test", 4);

  OTTER_ASSERT(true);

  OTTER_TEST_END();
}

OTTER_TEST(string_append_null_string) {
  otter_string *str = NULL;
  otter_string_append(&str, "test", 4);

  OTTER_ASSERT(str == NULL);

  OTTER_TEST_END();
}

OTTER_TEST(string_append_null_append) {
  otter_string *str = otter_string_from_cstr(OTTER_TEST_ALLOCATOR, "hello");
  size_t original_len = otter_string_length(str);

  const size_t dummy_length = 5;
  otter_string_append(&str, NULL, dummy_length);

  OTTER_ASSERT(otter_string_length(str) == original_len);
  OTTER_ASSERT(strcmp(otter_string_cstr(str), "hello") == 0);

  OTTER_TEST_END(otter_string_free(str););
}

OTTER_TEST(string_append_cstr) {
  otter_string *str = otter_string_from_cstr(OTTER_TEST_ALLOCATOR, "hello");

  OTTER_ASSERT(str != NULL);
  otter_string_append_cstr(&str, " world");

  OTTER_ASSERT(strcmp(otter_string_cstr(str), "hello world") == 0);

  OTTER_TEST_END(otter_string_free(str););
}

OTTER_TEST(string_append_cstr_null) {
  otter_string *str = otter_string_from_cstr(OTTER_TEST_ALLOCATOR, "hello");
  size_t original_len = otter_string_length(str);

  otter_string_append_cstr(&str, NULL);

  OTTER_ASSERT(otter_string_length(str) == original_len);
  OTTER_ASSERT(strcmp(otter_string_cstr(str), "hello") == 0);

  OTTER_TEST_END(otter_string_free(str););
}

OTTER_TEST(string_cstr_null) {
  const char *result = otter_string_cstr(NULL);

  OTTER_ASSERT(result == NULL);

  OTTER_TEST_END();
}

OTTER_TEST(string_length_null) {
  size_t len = otter_string_length(NULL);

  OTTER_ASSERT(len == 0);

  OTTER_TEST_END();
}

OTTER_TEST(string_clear) {
  otter_string *str = otter_string_from_cstr(OTTER_TEST_ALLOCATOR, "hello");

  OTTER_ASSERT(str != NULL);
  OTTER_ASSERT(otter_string_length(str) == 5);

  otter_string_clear(str);

  OTTER_ASSERT(otter_string_length(str) == 0);
  OTTER_ASSERT(strcmp(otter_string_cstr(str), "") == 0);

  OTTER_TEST_END(otter_string_free(str););
}

OTTER_TEST(string_clear_null) {
  otter_string_clear(NULL);

  OTTER_ASSERT(true);

  OTTER_TEST_END();
}

OTTER_TEST(string_clear_and_append) {
  otter_string *str = otter_string_from_cstr(OTTER_TEST_ALLOCATOR, "hello");

  OTTER_ASSERT(str != NULL);
  otter_string_clear(str);
  otter_string_append_cstr(&str, "world");

  OTTER_ASSERT(otter_string_length(str) == 5);
  OTTER_ASSERT(strcmp(otter_string_cstr(str), "world") == 0);

  OTTER_TEST_END(otter_string_free(str););
}

OTTER_TEST(string_compare_equal) {
  otter_string *str1 = otter_string_from_cstr(OTTER_TEST_ALLOCATOR, "hello");
  otter_string *str2 = otter_string_from_cstr(OTTER_TEST_ALLOCATOR, "hello");

  OTTER_ASSERT(otter_string_compare(str1, str2) == 0);

  OTTER_TEST_END(otter_string_free(str1); otter_string_free(str2););
}

OTTER_TEST(string_compare_different) {
  otter_string *str1 = otter_string_from_cstr(OTTER_TEST_ALLOCATOR, "hello");
  otter_string *str2 = otter_string_from_cstr(OTTER_TEST_ALLOCATOR, "world");

  OTTER_ASSERT(otter_string_compare(str1, str2) != 0);

  OTTER_TEST_END(otter_string_free(str1); otter_string_free(str2););
}

OTTER_TEST(string_compare_different_length) {
  otter_string *str1 = otter_string_from_cstr(OTTER_TEST_ALLOCATOR, "hello");
  otter_string *str2 =
      otter_string_from_cstr(OTTER_TEST_ALLOCATOR, "hello world");

  OTTER_ASSERT(otter_string_compare(str1, str2) < 0);
  // NOLINTNEXTLINE(readability-suspicious-call-argument)
  OTTER_ASSERT(otter_string_compare(str2, str1) > 0);

  OTTER_TEST_END(otter_string_free(str1); otter_string_free(str2););
}

OTTER_TEST(string_compare_both_null) {
  OTTER_ASSERT(otter_string_compare(NULL, NULL) == 0);

  OTTER_TEST_END();
}

OTTER_TEST(string_compare_first_null) {
  otter_string *str = otter_string_from_cstr(OTTER_TEST_ALLOCATOR, "hello");

  OTTER_ASSERT(otter_string_compare(NULL, str) < 0);

  OTTER_TEST_END(otter_string_free(str););
}

OTTER_TEST(string_compare_second_null) {
  otter_string *str = otter_string_from_cstr(OTTER_TEST_ALLOCATOR, "hello");

  OTTER_ASSERT(otter_string_compare(str, NULL) > 0);

  OTTER_TEST_END(otter_string_free(str););
}

OTTER_TEST(string_compare_cstr_equal) {
  otter_string *str = otter_string_from_cstr(OTTER_TEST_ALLOCATOR, "hello");

  OTTER_ASSERT(otter_string_compare_cstr(str, "hello") == 0);

  OTTER_TEST_END(otter_string_free(str););
}

OTTER_TEST(string_compare_cstr_different) {
  otter_string *str = otter_string_from_cstr(OTTER_TEST_ALLOCATOR, "hello");

  OTTER_ASSERT(otter_string_compare_cstr(str, "world") != 0);

  OTTER_TEST_END(otter_string_free(str););
}

OTTER_TEST(string_compare_cstr_both_null) {
  OTTER_ASSERT(otter_string_compare_cstr(NULL, NULL) == 0);

  OTTER_TEST_END();
}

OTTER_TEST(string_compare_cstr_str_null) {
  OTTER_ASSERT(otter_string_compare_cstr(NULL, "hello") < 0);

  OTTER_TEST_END();
}

OTTER_TEST(string_compare_cstr_cstr_null) {
  otter_string *str = otter_string_from_cstr(OTTER_TEST_ALLOCATOR, "hello");

  OTTER_ASSERT(otter_string_compare_cstr(str, NULL) > 0);

  OTTER_TEST_END(otter_string_free(str););
}

OTTER_TEST(string_free_null) {
  otter_string_free(NULL);

  OTTER_ASSERT(true);

  OTTER_TEST_END();
}

static void *malloc_mock(otter_allocator * /* unused */, size_t /*unused */) {
  return NULL;
}

OTTER_TEST(string_create_malloc_fails) {
  otter_allocator_vtable vtable = {
      .malloc = malloc_mock,
      .realloc = NULL,
      .free = NULL,
  };
  otter_allocator allocator = {
      .vtable = &vtable,
  };

  const size_t test_length = 5;
  otter_string *str = otter_string_create(&allocator, "hello", test_length);

  OTTER_ASSERT(str == NULL);

  OTTER_TEST_END();
}

OTTER_TEST(string_format_malloc_fails) {
  otter_allocator_vtable vtable = {
      .malloc = malloc_mock,
      .realloc = NULL,
      .free = NULL,
  };
  otter_allocator allocator = {
      .vtable = &vtable,
  };

  otter_string *str = otter_string_format(&allocator, "hello %s", "world");
  OTTER_ASSERT(str == NULL);
  OTTER_TEST_END();
}

static void *realloc_mock(otter_allocator * /*unused */, void * /*unused */,
                          size_t /*unused*/) {
  return NULL;
}

OTTER_TEST(string_append_realloc_fails) {
  otter_allocator_vtable vtable = {
      .malloc = OTTER_TEST_ALLOCATOR->vtable->malloc,
      .realloc = realloc_mock,
      .free = OTTER_TEST_ALLOCATOR->vtable->free,
  };
  otter_allocator allocator = {
      .vtable = &vtable,
  };

  otter_string *str = otter_string_create(&allocator, "a", 1);
  OTTER_ASSERT(str != NULL);

  size_t original_capacity = otter_string_capacity(str);
  size_t original_size = otter_string_length(str);

  char large_append[LARGE_BUFFER_SIZE];
  memset(large_append, 'b', sizeof(large_append) - 1);
  large_append[sizeof(large_append) - 1] = '\0';

  otter_string_append(&str, large_append, sizeof(large_append) - 1);

  OTTER_ASSERT(str != NULL);
  OTTER_ASSERT(otter_string_length(str) == original_size);
  OTTER_ASSERT(otter_string_capacity(str) == original_capacity);
  OTTER_ASSERT(strcmp(otter_string_cstr(str), "a") == 0);

  OTTER_TEST_END(otter_free(OTTER_TEST_ALLOCATOR, str););
}
