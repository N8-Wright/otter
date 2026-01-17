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
#include "otter_filesystem.h"
#include "otter_inc.h"
#include "otter_logger.h"
#include "otter_target.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#define CC_FLAGS_COMMON                                                        \
  "-std=gnu23 -Wall -Wextra -Werror -Wformat=2 -Wformat-security -Wundef "     \
  "-Wmissing-field-initializers -Wmissing-prototypes -Wmissing-declarations "  \
  "-Wshadow -Wcast-qual -Wcast-align -Wconversion -Wsign-conversion "          \
  "-Wfloat-equal -Winit-self -Wduplicated-cond -Wduplicated-branches "         \
  "-Wlogical-op -Wnull-dereference -Wold-style-definition -Wredundant-decls "  \
  "-Wmissing-include-dirs -Wformat-nonliteral -Wunused -Wuninitialized "       \
  "-Wmaybe-uninitialized -Wdeprecated-declarations -Wimplicit-fallthrough "    \
  "-Wformat-truncation "

#define LL_FLAGS_COMMON "-Wl,-z,relro -Wl,-z,now -Wl,-z,defs -Wl,--warn-common "

#define LL_FLAGS_DEBUG LL_FLAGS_COMMON ""
#define CC_FLAGS_DEBUG                                                         \
  CC_FLAGS_COMMON "-O0 -g -fsanitize=address,undefined,leak "

#define CC_FLAGS_RELEASE CC_FLAGS_COMMON "-O3 -D_FORTIFY_SOURCE=3 "
#define LL_FLAGS_RELEASE LL_FLAGS_COMMON "-flto "

#define CC_FLAGS_COVERAGE                                                      \
  CC_FLAGS_COMMON "-O0 -g -fprofile-arcs -ftest-coverage "
#define LL_FLAGS_COVERAGE LL_FLAGS_COMMON ""

#define CC_FLAGS_ASAN                                                          \
  CC_FLAGS_COMMON "-O0 -g -fsanitize=address,undefined,leak "
#define LL_FLAGS_ASAN LL_FLAGS_COMMON "-fsanitize=address,undefined,leak "

typedef enum {
  BUILD_MODE_DEBUG,
  BUILD_MODE_RELEASE,
} build_mode;

typedef struct {
  const char *suffix;
  const char *cc_flags;
  const char *ll_flags;
} build_config;

static void print_usage(const char *prog) {
  fprintf(stderr, "Usage: %s [--debug | --release]\n", prog);
  fprintf(
      stderr,
      "  --debug    Build with debug flags, ASAN, and coverage (default)\n");
  fprintf(stderr, "  --release  Build with optimized release flags\n");
}

#define NAME_BUF_SIZE 64
#define FLAGS_BUF_SIZE 2048

static void build_program_and_tests(otter_allocator *allocator,
                                    otter_filesystem *filesystem,
                                    otter_logger *logger,
                                    const build_config *config) {
  const char *sfx = config->suffix;
  const char *cc = config->cc_flags;
  const char *ll = config->ll_flags;

  /* Object files: name{suffix}.o */
  char allocator_o[NAME_BUF_SIZE], string_o[NAME_BUF_SIZE];
  char array_o[NAME_BUF_SIZE], cstring_o[NAME_BUF_SIZE];
  char logger_o[NAME_BUF_SIZE], file_o[NAME_BUF_SIZE];
  char filesystem_o[NAME_BUF_SIZE], target_o[NAME_BUF_SIZE];
  char token_o[NAME_BUF_SIZE], node_o[NAME_BUF_SIZE];
  char lexer_o[NAME_BUF_SIZE], parser_o[NAME_BUF_SIZE];
  char bytecode_o[NAME_BUF_SIZE], vm_o[NAME_BUF_SIZE];
  char test_o[NAME_BUF_SIZE];

  snprintf(allocator_o, NAME_BUF_SIZE, "otter_allocator%s.o", sfx);
  snprintf(string_o, NAME_BUF_SIZE, "otter_string%s.o", sfx);
  snprintf(array_o, NAME_BUF_SIZE, "otter_array%s.o", sfx);
  snprintf(cstring_o, NAME_BUF_SIZE, "otter_cstring%s.o", sfx);
  snprintf(logger_o, NAME_BUF_SIZE, "otter_logger%s.o", sfx);
  snprintf(file_o, NAME_BUF_SIZE, "otter_file%s.o", sfx);
  snprintf(filesystem_o, NAME_BUF_SIZE, "otter_filesystem%s.o", sfx);
  snprintf(target_o, NAME_BUF_SIZE, "otter_target%s.o", sfx);
  snprintf(token_o, NAME_BUF_SIZE, "otter_token%s.o", sfx);
  snprintf(node_o, NAME_BUF_SIZE, "otter_node%s.o", sfx);
  snprintf(lexer_o, NAME_BUF_SIZE, "otter_lexer%s.o", sfx);
  snprintf(parser_o, NAME_BUF_SIZE, "otter_parser%s.o", sfx);
  snprintf(bytecode_o, NAME_BUF_SIZE, "otter_bytecode%s.o", sfx);
  snprintf(vm_o, NAME_BUF_SIZE, "otter_vm%s.o", sfx);
  snprintf(test_o, NAME_BUF_SIZE, "otter_test%s.o", sfx);

  OTTER_CLEANUP(otter_target_free_p)
  otter_target *otter_allocator_obj =
      otter_target_create_c_object(allocator_o, cc, allocator, filesystem,
                                   logger, "otter_allocator.c", NULL);

  OTTER_CLEANUP(otter_target_free_p)
  otter_target *otter_string_obj = otter_target_create_c_object(
      string_o, cc, allocator, filesystem, logger, "otter_string.c", NULL);
  otter_target_add_dependency(otter_string_obj, otter_allocator_obj);

  OTTER_CLEANUP(otter_target_free_p)
  otter_target *otter_array_obj = otter_target_create_c_object(
      array_o, cc, allocator, filesystem, logger, "otter_array.c", NULL);
  otter_target_add_dependency(otter_array_obj, otter_allocator_obj);

  OTTER_CLEANUP(otter_target_free_p)
  otter_target *otter_cstring_obj = otter_target_create_c_object(
      cstring_o, cc, allocator, filesystem, logger, "otter_cstring.c", NULL);
  otter_target_add_dependency(otter_cstring_obj, otter_allocator_obj);

  OTTER_CLEANUP(otter_target_free_p)
  otter_target *otter_logger_obj = otter_target_create_c_object(
      logger_o, cc, allocator, filesystem, logger, "otter_logger.c", NULL);
  otter_target_add_dependency(otter_logger_obj, otter_cstring_obj);
  otter_target_add_dependency(otter_logger_obj, otter_array_obj);
  otter_target_add_dependency(otter_logger_obj, otter_allocator_obj);

  OTTER_CLEANUP(otter_target_free_p)
  otter_target *otter_file_obj = otter_target_create_c_object(
      file_o, cc, allocator, filesystem, logger, "otter_file.c", NULL);

  OTTER_CLEANUP(otter_target_free_p)
  otter_target *otter_filesystem_obj =
      otter_target_create_c_object(filesystem_o, cc, allocator, filesystem,
                                   logger, "otter_filesystem.c", NULL);
  otter_target_add_dependency(otter_filesystem_obj, otter_file_obj);
  otter_target_add_dependency(otter_filesystem_obj, otter_allocator_obj);

  OTTER_CLEANUP(otter_target_free_p)
  otter_target *otter_target_obj = otter_target_create_c_object(
      target_o, cc, allocator, filesystem, logger, "otter_target.c", NULL);
  otter_target_add_dependency(otter_target_obj, otter_allocator_obj);
  otter_target_add_dependency(otter_target_obj, otter_array_obj);
  otter_target_add_dependency(otter_target_obj, otter_filesystem_obj);
  otter_target_add_dependency(otter_target_obj, otter_logger_obj);
  otter_target_add_dependency(otter_target_obj, otter_string_obj);

  OTTER_CLEANUP(otter_target_free_p)
  otter_target *otter_token_obj = otter_target_create_c_object(
      token_o, cc, allocator, filesystem, logger, "otter_token.c", NULL);
  otter_target_add_dependency(otter_token_obj, otter_allocator_obj);

  OTTER_CLEANUP(otter_target_free_p)
  otter_target *otter_node_obj = otter_target_create_c_object(
      node_o, cc, allocator, filesystem, logger, "otter_node.c", NULL);
  otter_target_add_dependency(otter_node_obj, otter_allocator_obj);
  otter_target_add_dependency(otter_node_obj, otter_array_obj);

  OTTER_CLEANUP(otter_target_free_p)
  otter_target *otter_lexer_obj = otter_target_create_c_object(
      lexer_o, cc, allocator, filesystem, logger, "otter_lexer.c", NULL);
  otter_target_add_dependency(otter_lexer_obj, otter_array_obj);
  otter_target_add_dependency(otter_lexer_obj, otter_cstring_obj);

  OTTER_CLEANUP(otter_target_free_p)
  otter_target *otter_parser_obj = otter_target_create_c_object(
      parser_o, cc, allocator, filesystem, logger, "otter_parser.c", NULL);
  otter_target_add_dependency(otter_parser_obj, otter_allocator_obj);
  otter_target_add_dependency(otter_parser_obj, otter_logger_obj);
  otter_target_add_dependency(otter_parser_obj, otter_node_obj);
  otter_target_add_dependency(otter_parser_obj, otter_cstring_obj);
  otter_target_add_dependency(otter_parser_obj, otter_token_obj);

  OTTER_CLEANUP(otter_target_free_p)
  otter_target *otter_bytecode_obj = otter_target_create_c_object(
      bytecode_o, cc, allocator, filesystem, logger, "otter_bytecode.c", NULL);

  OTTER_CLEANUP(otter_target_free_p)
  otter_target *otter_vm_obj = otter_target_create_c_object(
      vm_o, cc, allocator, filesystem, logger, "otter_vm.c", NULL);
  otter_target_add_dependency(otter_vm_obj, otter_allocator_obj);
  otter_target_add_dependency(otter_vm_obj, otter_logger_obj);
  otter_target_add_dependency(otter_vm_obj, otter_bytecode_obj);

  /* Main executable */
  char exe_name[NAME_BUF_SIZE];
  char exe_flags[FLAGS_BUF_SIZE];
  snprintf(exe_name, NAME_BUF_SIZE, "otter%s", sfx);
  snprintf(exe_flags, FLAGS_BUF_SIZE, "%s%s", cc, ll);

  OTTER_CLEANUP(otter_target_free_p)
  otter_target *otter_exe = otter_target_create_c_executable(
      exe_name, exe_flags, allocator, filesystem, logger,
      (const char *[]){"otter.c", NULL},
      (otter_target *[]){otter_vm_obj, NULL});
  otter_target_execute(otter_exe);

  /* Test driver */
  OTTER_CLEANUP(otter_target_free_p)
  otter_target *otter_test_obj = otter_target_create_c_object(
      test_o, cc, allocator, filesystem, logger, "otter_test.c", NULL);
  otter_target_add_dependency(otter_test_obj, otter_allocator_obj);

  char test_exe_name[NAME_BUF_SIZE];
  snprintf(test_exe_name, NAME_BUF_SIZE, "otter_test%s", sfx);

  OTTER_CLEANUP(otter_target_free_p)
  otter_target *otter_test_driver = otter_target_create_c_executable(
      test_exe_name, exe_flags, allocator, filesystem, logger,
      (const char *[]){"otter_test_driver.c", NULL},
      (otter_target *[]){otter_allocator_obj, NULL});
  otter_target_execute(otter_test_driver);

  /* Test shared objects */
  char cstring_tests_name[NAME_BUF_SIZE];
  snprintf(cstring_tests_name, NAME_BUF_SIZE, "otter_cstring_tests%s.so", sfx);

  OTTER_CLEANUP(otter_target_free_p)
  otter_target *otter_cstring_tests = otter_target_create_c_shared_object(
      cstring_tests_name, cc, allocator, filesystem, logger,
      (const char *[]){"otter_cstring_tests.c", NULL},
      (otter_target *[]){otter_test_obj, otter_cstring_obj, NULL});
  otter_target_execute(otter_cstring_tests);

  char array_tests_name[NAME_BUF_SIZE];
  snprintf(array_tests_name, NAME_BUF_SIZE, "otter_array_tests%s.so", sfx);

  OTTER_CLEANUP(otter_target_free_p)
  otter_target *otter_array_tests = otter_target_create_c_shared_object(
      array_tests_name, exe_flags, allocator, filesystem, logger,
      (const char *[]){"otter_array_tests.c", NULL},
      (otter_target *[]){otter_test_obj, otter_array_obj, NULL});
  otter_target_execute(otter_array_tests);

  char lexer_tests_name[NAME_BUF_SIZE];
  snprintf(lexer_tests_name, NAME_BUF_SIZE, "otter_lexer_tests%s.so", sfx);

  OTTER_CLEANUP(otter_target_free_p)
  otter_target *otter_lexer_tests = otter_target_create_c_shared_object(
      lexer_tests_name, exe_flags, allocator, filesystem, logger,
      (const char *[]){"otter_lexer_tests.c", NULL},
      (otter_target *[]){otter_test_obj, otter_lexer_obj, otter_token_obj,
                         NULL});
  otter_target_execute(otter_lexer_tests);

  char parser_tests_name[NAME_BUF_SIZE];
  snprintf(parser_tests_name, NAME_BUF_SIZE, "otter_parser_tests%s.so", sfx);

  OTTER_CLEANUP(otter_target_free_p)
  otter_target *otter_parser_tests = otter_target_create_c_shared_object(
      parser_tests_name, exe_flags, allocator, filesystem, logger,
      (const char *[]){"otter_parser_tests.c", NULL},
      (otter_target *[]){otter_test_obj, otter_cstring_obj, otter_node_obj,
                         otter_parser_obj, NULL});
  otter_target_execute(otter_parser_tests);

  char parser_int_tests_name[NAME_BUF_SIZE];
  snprintf(parser_int_tests_name, NAME_BUF_SIZE,
           "otter_parser_integration_tests%s.so", sfx);

  OTTER_CLEANUP(otter_target_free_p)
  otter_target *otter_parser_integration_tests =
      otter_target_create_c_shared_object(
          parser_int_tests_name, exe_flags, allocator, filesystem, logger,
          (const char *[]){"otter_parser_integration_tests.c", NULL},
          (otter_target *[]){otter_test_obj, otter_lexer_obj, otter_node_obj,
                             otter_parser_obj, NULL});
  otter_target_execute(otter_parser_integration_tests);
}

int main(int argc, char *argv[]) {
  /* Parse command line arguments */
  build_mode mode = BUILD_MODE_DEBUG; /* default */

  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--debug") == 0) {
      mode = BUILD_MODE_DEBUG;
    } else if (strcmp(argv[i], "--release") == 0) {
      mode = BUILD_MODE_RELEASE;
    } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
      print_usage(argv[0]);
      return 0;
    } else {
      fprintf(stderr, "Unknown option: %s\n", argv[i]);
      print_usage(argv[0]);
      return 1;
    }
  }

  OTTER_CLEANUP(otter_allocator_free_p)
  otter_allocator *allocator = otter_allocator_create();

  OTTER_CLEANUP(otter_logger_free_p)
  otter_logger *logger = otter_logger_create(allocator, OTTER_LOG_LEVEL_INFO);
  otter_logger_add_sink(logger, otter_logger_console_sink);

  OTTER_CLEANUP(otter_filesystem_free_p)
  otter_filesystem *filesystem = otter_filesystem_create(allocator);

  /* Build the build system (always with base flags) */
  OTTER_CLEANUP(otter_target_free_p)
  otter_target *otter_allocator_obj = otter_target_create_c_object(
      "otter_allocator.o", CC_FLAGS_RELEASE, allocator, filesystem, logger,
      "otter_allocator.c", NULL);

  OTTER_CLEANUP(otter_target_free_p)
  otter_target *otter_string_obj = otter_target_create_c_object(
      "otter_string.o", CC_FLAGS_RELEASE, allocator, filesystem, logger,
      "otter_string.c", NULL);
  otter_target_add_dependency(otter_string_obj, otter_allocator_obj);

  OTTER_CLEANUP(otter_target_free_p)
  otter_target *otter_array_obj =
      otter_target_create_c_object("otter_array.o", CC_FLAGS_RELEASE, allocator,
                                   filesystem, logger, "otter_array.c", NULL);
  otter_target_add_dependency(otter_array_obj, otter_allocator_obj);

  OTTER_CLEANUP(otter_target_free_p)
  otter_target *otter_cstring_obj = otter_target_create_c_object(
      "otter_cstring.o", CC_FLAGS_RELEASE, allocator, filesystem, logger,
      "otter_cstring.c", NULL);
  otter_target_add_dependency(otter_cstring_obj, otter_allocator_obj);

  OTTER_CLEANUP(otter_target_free_p)
  otter_target *otter_logger_obj = otter_target_create_c_object(
      "otter_logger.o", CC_FLAGS_RELEASE, allocator, filesystem, logger,
      "otter_logger.c", NULL);
  otter_target_add_dependency(otter_logger_obj, otter_cstring_obj);
  otter_target_add_dependency(otter_logger_obj, otter_array_obj);
  otter_target_add_dependency(otter_logger_obj, otter_allocator_obj);

  OTTER_CLEANUP(otter_target_free_p)
  otter_target *otter_file_obj =
      otter_target_create_c_object("otter_file.o", CC_FLAGS_RELEASE, allocator,
                                   filesystem, logger, "otter_file.c", NULL);

  OTTER_CLEANUP(otter_target_free_p)
  otter_target *otter_filesystem_obj = otter_target_create_c_object(
      "otter_filesystem.o", CC_FLAGS_RELEASE, allocator, filesystem, logger,
      "otter_filesystem.c", NULL);
  otter_target_add_dependency(otter_filesystem_obj, otter_file_obj);
  otter_target_add_dependency(otter_filesystem_obj, otter_allocator_obj);

  OTTER_CLEANUP(otter_target_free_p)
  otter_target *otter_target_obj = otter_target_create_c_object(
      "otter_target.o", CC_FLAGS_RELEASE, allocator, filesystem, logger,
      "otter_target.c", NULL);
  otter_target_add_dependency(otter_target_obj, otter_allocator_obj);
  otter_target_add_dependency(otter_target_obj, otter_array_obj);
  otter_target_add_dependency(otter_target_obj, otter_filesystem_obj);
  otter_target_add_dependency(otter_target_obj, otter_logger_obj);
  otter_target_add_dependency(otter_target_obj, otter_string_obj);

  char make_flags[FLAGS_BUF_SIZE];
  snprintf(make_flags, FLAGS_BUF_SIZE, "%s -lgnutls", CC_FLAGS_RELEASE);
  OTTER_CLEANUP(otter_target_free_p)
  otter_target *otter_make_exe = otter_target_create_c_executable(
      "otter_make", make_flags, allocator, filesystem, logger,
      (const char *[]){"otter_make.c", NULL},
      (otter_target *[]){otter_allocator_obj, otter_filesystem_obj,
                         otter_logger_obj, otter_target_obj, NULL});
  otter_target_execute(otter_make_exe);

  if (mode == BUILD_MODE_DEBUG) {
    build_config configs[] = {
        {"", CC_FLAGS_DEBUG, LL_FLAGS_DEBUG},
        {"_coverage", CC_FLAGS_COVERAGE, LL_FLAGS_COVERAGE},
    };

    for (size_t i = 0; i < sizeof(configs) / sizeof(configs[0]); i++) {
      build_program_and_tests(allocator, filesystem, logger, &configs[i]);
    }
  } else {
    /* Release mode: only build the optimized version */
    build_config config = {"", CC_FLAGS_RELEASE, LL_FLAGS_COVERAGE};
    build_program_and_tests(allocator, filesystem, logger, &config);
  }

  return 0;
}
