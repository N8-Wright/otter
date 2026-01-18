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
#include "otter_test.h"
#include <limits.h>
void otter_test_list(otter_allocator *allocator, const char ***test_names,
                     int *test_count) {
  if (allocator == NULL || test_names == NULL || test_count == NULL) {
    return;
  }

  /* Linker guarantees __start and __stop symbols point to the same section */
  /* NOLINTNEXTLINE(clang-analyzer-security.PointerSub) */
  intptr_t num_tests = __stop_otter_test_section - __start_otter_test_section;
  if (num_tests > (intptr_t)INT_MAX) {
    return;
  }

  *test_names = otter_malloc(allocator, sizeof(char *) * (size_t)num_tests);
  if (*test_names == NULL) {
    *test_count = 0;
    return;
  }

  *test_count = (int)num_tests;
  for (intptr_t i = 0; i < num_tests; ++i) {
    (*test_names)[i] = __start_otter_test_section[i].name;
  }
}
