#include "otter_test.h"
#include <limits.h>
void otter_test_list(otter_allocator *allocator, const char ***test_names,
                     int *test_count) {
  if (allocator == NULL || test_names == NULL || test_count == NULL) {
    return;
  }

  intptr_t n = __stop_otter_test_section - __start_otter_test_section;
  if (n > (intptr_t)INT_MAX) {
    return;
  }

  *test_names = otter_malloc(allocator, sizeof(char *) * (size_t)n);
  if (*test_names == NULL) {
    *test_count = 0;
    return;
  }

  *test_count = (int)n;
  for (intptr_t i = 0; i < n; ++i) {
    (*test_names)[i] = __start_otter_test_section[i].name;
  }
}
