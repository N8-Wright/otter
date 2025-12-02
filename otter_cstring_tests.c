#include "otter_test.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

OTTER_TEST(foobar) { return true; }
OTTER_TEST(barfoo) { return false; }
