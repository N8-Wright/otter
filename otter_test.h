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
#include <stdbool.h>
typedef struct otter_test_context {
  char *failed_expression;
  int failed_line;
} otter_test_context;

typedef void (*otter_test_list_fn)(const char ***test_names, int *test_count);
typedef bool (*otter_test_fn)(otter_test_context *ctx);

#endif /* OTTER_TEST_H_ */
