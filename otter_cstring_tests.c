#include "otter_test.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
OTTER_TEST(foo) {
  void *foo = malloc(10);
  void *bar = malloc(20);
  OTTER_ASSERT(1 == 0);
  OTTER_ASSERT(true);

  OTTER_TEST_END(free(foo); free(bar););
}

OTTER_TEST(foobar) { return true; }
OTTER_TEST(barfoo) { return false; }
