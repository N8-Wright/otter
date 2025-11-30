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
    "Show license information"};

void print_usage(const char *program_name) {
  printf("Usage: %s [OPTIONS] <shared_object.so>\nOptions:\n", program_name);
  for (size_t i = 0; long_opts[i].name; i++) {
    printf("  -%c, --%s", long_opts[i].val, long_opts[i].name);
    if (long_opts[i].has_arg == required_argument) {
      printf(" <%s>", long_opts[i].name);
    }
    printf("\n    %s\n", opt_desc[i]);
  }
}

int main(int argc, char *argv[]) {
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
    return EXIT_FAILURE;
  }

  void *func = dlsym(handle, "otter_test_list");
  if (!func) {
    fprintf(stderr, "dlsym error: %s\n", dlerror());
    dlclose(handle);
    return EXIT_FAILURE;
  }

  otter_test_list_fn list_func;
  memcpy(&list_func, func, sizeof(otter_test_list_fn));
  int count = 0;
  char **testnames;
  list_func(&testnames, &count);

  if (!testnames || count == 0) {
    printf("No tests found.\n");
    dlclose(handle);
    return 0;
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
        void *fn = dlsym(handle, testnames[i]);
        if (!fn) {
          fprintf(stderr, "dlsym '%s': %s\n", testnames[i], dlerror());
          break;
        }

        otter_test_fn test_fn;
        memcpy(&test_fn, fn, sizeof(test_fn));
        printf("Running %s:\n", testnames[i]);
        otter_test_context ctx = {};
        bool passed = test_fn(&ctx);
        if (passed) {
          printf("Passed!\n");
        } else {
          printf("Failed :(\n");
        }
        break;
      }
    }
    if (!found) {
      printf("Test not found: %s\n", run_test_name);
    }
  } else {
    for (int i = 0; i < count; ++i) {
      void *fn = dlsym(handle, testnames[i]);
      if (!fn) {
        fprintf(stderr, "dlsym '%s': %s\n", testnames[i], dlerror());
        continue;
      }
      printf("Running %s:\n", testnames[i]);
      otter_test_fn test_fn;
      memcpy(&test_fn, fn, sizeof(test_fn));
      otter_test_context ctx = {};
      bool passed = test_fn(&ctx);
      if (passed) {
        printf("Passed!\n");
      } else {
        printf("Failed :(\n");
      }
    }
  }

  dlclose(handle);
  return EXIT_SUCCESS;
}
