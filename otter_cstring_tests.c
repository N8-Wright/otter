#include "otter_test.h"
#include <stddef.h>
static const char *tests[] = {
    "foo",
    "bar",
};

void otter_test_list(const char ***test_names, int *test_count) {
  if (test_names == NULL || test_count == NULL) {
    return;
  }

  *test_names = tests;
  *test_count = sizeof(tests) / sizeof(tests[0]);
}

bool foo(otter_test_context *) { return true; }

bool bar(otter_test_context *) { return false; }
