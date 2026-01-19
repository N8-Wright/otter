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
#include "otter_allocator.h"
#include "otter_term_colors.h"
#include "otter_test.h"
#include <dlfcn.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *license =
    "otter Copyright (C) 2025 Nathaniel Wright\n"
    "This program is free software: you can redistribute it and/or modify\n"
    "it under the terms of the GNU General Public License as published by\n"
    "the Free Software Foundation, either version 3 of the License, or\n"
    "(at your option) any later version.\n\n"
    "This program is distributed in the hope that it will be useful,\n"
    "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
    "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
    "GNU General Public License for more details.\n\n"
    "You should have received a copy of the GNU General Public License\n"
    "along with this program.  If not, see <https://www.gnu.org/licenses/>.";

static struct option long_opts[] = {{"list", no_argument, 0, 'l'},
                                    {"test", required_argument, 0, 't'},
                                    {"help", no_argument, 0, 'h'},
                                    {"license", no_argument, 0, 'L'},
                                    {0, 0, 0, 0}};

static const char *opt_desc[] = {
    "List available tests", "Run a single test by name", "Print this message",
    "Show license information", ""};

static void print_usage(const char *program_name) {
  printf("Usage: %s [OPTIONS] <shared_object.so>\nOptions:\n", program_name);
  for (size_t i = 0; long_opts[i].name; i++) {
    printf("  -%c, --%s", long_opts[i].val, long_opts[i].name);
    if (long_opts[i].has_arg == required_argument) {
      printf(" <%s>", long_opts[i].name);
    }
    printf("\n    %s\n", opt_desc[i]);
  }
}

static void run_test(void *handle, otter_allocator *allocator,
                     const char *testname) {
  otter_test_fn test_fn = dlsym(handle, testname);
  if (!test_fn) {
    fprintf(stderr, "dlsym '%s': %s\n", testname, dlerror());
    return;
  }

  printf("Running " OTTER_TERM_CYAN("%s") "...\n", testname);
  otter_test_context ctx = {
      .allocator = allocator,
      .test_status = true,
  };
  bool passed = test_fn(&ctx);
  if (passed) {
    printf(OTTER_TERM_GREEN("passed") "\n");
  } else {
    if (ctx.failed_expression != NULL) {
      printf(OTTER_TERM_RED("failed") "\n%s:%d: '%s'\n", ctx.failed_file,
             ctx.failed_line, ctx.failed_expression);
    } else {
      printf(OTTER_TERM_RED("failed") "\n");
    }
  }
}

int main(int argc, char *argv[]) {
  otter_allocator *allocator = otter_allocator_create();
  int opt;
  bool list_flag = false;
  bool show_license = false;
  bool show_help = false;
  char *run_test_name = NULL;

  while ((opt = getopt_long(argc, argv, "lt:hL", long_opts, NULL)) != -1) {
    switch (opt) {
    case 'l':
      list_flag = true;
      break;
    case 'L':
      show_license = true;
      break;
    case 't':
      run_test_name = optarg;
      break;
    case 'h':
    default:
      show_help = true;
      break;
    }
  }

  if (show_license) {
    printf("%s\n", license);
  }

  if (show_help) {
    print_usage(argv[0]);
    return EXIT_SUCCESS;
  }

  if (optind >= argc) {
    fprintf(stderr, "Expected path to shared object.\n");
    print_usage(argv[0]);
    return EXIT_FAILURE;
  }

  const char *so_path = argv[optind];
  void *handle = dlopen(so_path, RTLD_LAZY);
  if (!handle) {
    fprintf(stderr, "dlopen error: %s\n", dlerror());
    otter_allocator_free(allocator);
    return EXIT_FAILURE;
  }

  otter_test_list_fn list_func = dlsym(handle, "otter_test_list");
  if (list_func == NULL) {
    fprintf(stderr, "Unable to find \"otter_test_list\" function");
    otter_allocator_free(allocator);
    dlclose(handle);
    return EXIT_FAILURE;
  }

  int count = 0;
  const char **testnames = NULL;
  list_func(allocator, &testnames, &count);

  if (!testnames || count == 0) {
    printf("No tests found.\n");
    dlclose(handle);
    otter_allocator_free(allocator);
    return EXIT_SUCCESS;
  }
  if (list_flag) {
    printf("Tests:\n");
    for (int i = 0; i < count; i++) {
      printf("\t * %s\n", testnames[i]);
    }
  } else if (run_test_name) {
    int found = 0;
    for (int i = 0; i < count; ++i) {
      if (strcmp(testnames[i], run_test_name) == 0) {
        found = 1;
        run_test(handle, allocator, testnames[i]);
        break;
      }
    }
    if (!found) {
      printf("Test not found: %s\n", run_test_name);
    }
  } else {
    for (int i = 0; i < count; ++i) {
      run_test(handle, allocator, testnames[i]);
    }
  }

  otter_free(allocator, testnames);
  dlclose(handle);
  otter_allocator_free(allocator);
  return EXIT_SUCCESS;
}
