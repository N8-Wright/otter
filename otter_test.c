#include "otter_test.h"

void otter_test_list(const char ***test_names, int *test_count) {
  if (test_names == NULL || test_count == NULL) {
    return;
  }

  intptr_t n = __stop_otter_test_section - __start_otter_test_section;
  static const char *names[128]; // or dynamically allocate!
  for (intptr_t i = 0; i < n; ++i) {
    names[i] = __start_otter_test_section[i].name;
  }

  *test_count = (int)n;
  *test_names = names;
}
