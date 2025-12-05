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
#ifndef OTTER_TEST_H_
#define OTTER_TEST_H_
#include "otter_allocator.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
typedef struct otter_test_context {
  char *failed_expression;
  int failed_line;
} otter_test_context;

typedef void (*otter_test_list_fn)(otter_allocator *, const char ***test_names,
                                   int *test_count);
typedef bool (*otter_test_fn)(otter_test_context *ctx);

typedef struct otter_test_entry {
  const char *name;
  otter_test_fn test;
} otter_test_entry;

#define OTTER_TEST_SECTION_NAME otter_test_section

#define OTTER_TEST_STRINGIFY(arg) #arg
#define OTTER_TEST_EXPAND_AND_STRINGIFY(arg) OTTER_TEST_STRINGIFY(arg)

#define OTTER_TEST_DECLARE_ENTRY_(name)                                        \
  extern struct otter_test_entry __start_##name[];                             \
  extern struct otter_test_entry __stop_##name[]
#define OTTER_TEST_DECLARE_ENTRY(name) OTTER_TEST_DECLARE_ENTRY_(name)

#define OTTER_TEST_CONTEXT_VARNAME otter_test_ctx
#define OTTER_TEST(name)                                                       \
  bool name(otter_test_context *);                                             \
  __attribute__((used, section(OTTER_TEST_EXPAND_AND_STRINGIFY(                \
                           OTTER_TEST_SECTION_NAME)))) static otter_test_entry \
      name##_entry = {#name, name};                                            \
  bool name(__attribute__((unused)) otter_test_context *OTTER_TEST_CONTEXT_NAME)

OTTER_TEST_DECLARE_ENTRY(OTTER_TEST_SECTION_NAME);
void otter_test_list(otter_allocator *allocator, const char ***test_names,
                     int *test_count);
#endif /* OTTER_TEST_H_ */
