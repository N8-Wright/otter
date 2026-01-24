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
#include "otter/allocator.h"
#include "otter/cstring.h"
#include "otter/filesystem.h"
#include "otter/inc.h"
#include "otter/logger.h"
#include "otter/string.h"
#include "otter/target.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#define CC_INCLUDE_FLAGS "-I ./include "
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
  const char *src_dir;
  const char *out_dir;
} build_config;

static void print_usage(const char *prog) {
  fprintf(stderr, "Usage: %s [--debug | --release]\n", prog);
  fprintf(
      stderr,
      "  --debug    Build with debug flags, ASAN, and coverage (default)\n");
  fprintf(stderr, "  --release  Build with optimized release flags\n");
}

static void build_program_and_tests(otter_allocator *allocator,
                                    otter_filesystem *filesystem,
                                    otter_logger *logger,
                                    const build_config *config) {

  const char *cc_flags = config->cc_flags;
  const char *ll_flags = config->ll_flags;

  /* Object files: name{suffix}.o */
  OTTER_CLEANUP(otter_string_free_p)
  otter_string *allocator_o_file = otter_string_format(
      allocator, "%s/allocator%s.o", config->out_dir, config->suffix);
  if (allocator_o_file == NULL) {
    return;
  }

  OTTER_CLEANUP(otter_string_free_p)
  otter_string *string_o_file = otter_string_format(
      allocator, "%s/string%s.o", config->out_dir, config->suffix);
  if (string_o_file == NULL) {
    return;
  }

  OTTER_CLEANUP(otter_string_free_p)
  otter_string *array_o_file = otter_string_format(
      allocator, "%s/array%s.o", config->out_dir, config->suffix);
  if (array_o_file == NULL) {
    return;
  }

  OTTER_CLEANUP(otter_string_free_p)
  otter_string *cstring_o_file = otter_string_format(
      allocator, "%s/cstring%s.o", config->out_dir, config->suffix);
  if (cstring_o_file == NULL) {
    return;
  }

  OTTER_CLEANUP(otter_string_free_p)
  otter_string *logger_o_file = otter_string_format(
      allocator, "%s/logger%s.o", config->out_dir, config->suffix);
  if (logger_o_file == NULL) {
    return;
  }

  OTTER_CLEANUP(otter_string_free_p)
  otter_string *file_o_file = otter_string_format(
      allocator, "%s/file%s.o", config->out_dir, config->suffix);
  if (file_o_file == NULL) {
    return;
  }

  OTTER_CLEANUP(otter_string_free_p)
  otter_string *filesystem_o_file = otter_string_format(
      allocator, "%s/filesystem%s.o", config->out_dir, config->suffix);
  if (filesystem_o_file == NULL) {
    return;
  }

  OTTER_CLEANUP(otter_string_free_p)
  otter_string *target_o_file = otter_string_format(
      allocator, "%s/target%s.o", config->out_dir, config->suffix);
  if (target_o_file == NULL) {
    return;
  }

  OTTER_CLEANUP(otter_string_free_p)
  otter_string *token_o_file = otter_string_format(
      allocator, "%s/token%s.o", config->out_dir, config->suffix);
  if (token_o_file == NULL) {
    return;
  }

  OTTER_CLEANUP(otter_string_free_p)
  otter_string *node_o_file = otter_string_format(
      allocator, "%s/node%s.o", config->out_dir, config->suffix);
  if (node_o_file == NULL) {
    return;
  }

  OTTER_CLEANUP(otter_string_free_p)
  otter_string *lexer_o_file = otter_string_format(
      allocator, "%s/lexer%s.o", config->out_dir, config->suffix);
  if (lexer_o_file == NULL) {
    return;
  }

  OTTER_CLEANUP(otter_string_free_p)
  otter_string *parser_o_file = otter_string_format(
      allocator, "%s/parser%s.o", config->out_dir, config->suffix);
  if (parser_o_file == NULL) {
    return;
  }

  OTTER_CLEANUP(otter_string_free_p)
  otter_string *bytecode_o_file = otter_string_format(
      allocator, "%s/bytecode%s.o", config->out_dir, config->suffix);
  if (bytecode_o_file == NULL) {
    return;
  }

  OTTER_CLEANUP(otter_string_free_p)
  otter_string *vm_o_file = otter_string_format(
      allocator, "%s/vm%s.o", config->out_dir, config->suffix);
  if (vm_o_file == NULL) {
    return;
  }

  OTTER_CLEANUP(otter_string_free_p)
  otter_string *test_o_file = otter_string_format(
      allocator, "%s/test%s.o", config->out_dir, config->suffix);
  if (test_o_file == NULL) {
    return;
  }

  OTTER_CLEANUP(otter_string_free_p)
  otter_string *allocator_src_file =
      otter_string_format(allocator, "%s/allocator.c", config->src_dir);
  if (allocator_src_file == NULL) {
    return;
  }

  OTTER_CLEANUP(otter_string_free_p)
  otter_string *string_src_file =
      otter_string_format(allocator, "%s/string.c", config->src_dir);
  if (string_src_file == NULL) {
    return;
  }

  OTTER_CLEANUP(otter_string_free_p)
  otter_string *array_src_file =
      otter_string_format(allocator, "%s/array.c", config->src_dir);
  if (array_src_file == NULL) {
    return;
  }

  OTTER_CLEANUP(otter_string_free_p)
  otter_string *cstring_src_file =
      otter_string_format(allocator, "%s/cstring.c", config->src_dir);
  if (cstring_src_file == NULL) {
    return;
  }

  OTTER_CLEANUP(otter_string_free_p)
  otter_string *logger_src_file =
      otter_string_format(allocator, "%s/logger.c", config->src_dir);
  if (logger_src_file == NULL) {
    return;
  }

  OTTER_CLEANUP(otter_string_free_p)
  otter_string *file_src_file =
      otter_string_format(allocator, "%s/file.c", config->src_dir);
  if (file_src_file == NULL) {
    return;
  }

  OTTER_CLEANUP(otter_string_free_p)
  otter_string *filesystem_src_file =
      otter_string_format(allocator, "%s/filesystem.c", config->src_dir);
  if (filesystem_src_file == NULL) {
    return;
  }

  OTTER_CLEANUP(otter_string_free_p)
  otter_string *target_src_file =
      otter_string_format(allocator, "%s/target.c", config->src_dir);
  if (target_src_file == NULL) {
    return;
  }

  OTTER_CLEANUP(otter_string_free_p)
  otter_string *token_src_file =
      otter_string_format(allocator, "%s/token.c", config->src_dir);
  if (token_src_file == NULL) {
    return;
  }

  OTTER_CLEANUP(otter_string_free_p)
  otter_string *node_src_file =
      otter_string_format(allocator, "%s/node.c", config->src_dir);
  if (node_src_file == NULL) {
    return;
  }

  OTTER_CLEANUP(otter_string_free_p)
  otter_string *lexer_src_file =
      otter_string_format(allocator, "%s/lexer.c", config->src_dir);
  if (lexer_src_file == NULL) {
    return;
  }

  OTTER_CLEANUP(otter_string_free_p)
  otter_string *parser_src_file =
      otter_string_format(allocator, "%s/parser.c", config->src_dir);
  if (parser_src_file == NULL) {
    return;
  }

  OTTER_CLEANUP(otter_string_free_p)
  otter_string *bytecode_src_file =
      otter_string_format(allocator, "%s/bytecode.c", config->src_dir);
  if (bytecode_src_file == NULL) {
    return;
  }

  OTTER_CLEANUP(otter_string_free_p)
  otter_string *vm_src_file =
      otter_string_format(allocator, "%s/vm.c", config->src_dir);
  if (vm_src_file == NULL) {
    return;
  }

  OTTER_CLEANUP(otter_string_free_p)
  otter_string *main_src_file =
      otter_string_format(allocator, "%s/otter.c", config->src_dir);
  if (main_src_file == NULL) {
    return;
  }

  OTTER_CLEANUP(otter_string_free_p)
  otter_string *test_src_file =
      otter_string_format(allocator, "%s/test.c", config->src_dir);
  if (test_src_file == NULL) {
    return;
  }

  OTTER_CLEANUP(otter_string_free_p)
  otter_string *test_driver_src_file =
      otter_string_format(allocator, "%s/test_driver.c", config->src_dir);
  if (test_driver_src_file == NULL) {
    return;
  }

  OTTER_CLEANUP(otter_string_free_p)
  otter_string *cstring_tests_src_file =
      otter_string_format(allocator, "%s/cstring_tests.c", config->src_dir);
  if (cstring_tests_src_file == NULL) {
    return;
  }

  OTTER_CLEANUP(otter_string_free_p)
  otter_string *string_tests_src_file =
      otter_string_format(allocator, "%s/string_tests.c", config->src_dir);
  if (string_tests_src_file == NULL) {
    return;
  }

  OTTER_CLEANUP(otter_string_free_p)
  otter_string *array_tests_src_file =
      otter_string_format(allocator, "%s/array_tests.c", config->src_dir);
  if (array_tests_src_file == NULL) {
    return;
  }

  OTTER_CLEANUP(otter_string_free_p)
  otter_string *lexer_tests_src_file =
      otter_string_format(allocator, "%s/lexer_tests.c", config->src_dir);
  if (lexer_tests_src_file == NULL) {
    return;
  }

  OTTER_CLEANUP(otter_string_free_p)
  otter_string *parser_tests_src_file =
      otter_string_format(allocator, "%s/parser_tests.c", config->src_dir);
  if (parser_tests_src_file == NULL) {
    return;
  }

  OTTER_CLEANUP(otter_string_free_p)
  otter_string *parser_integration_tests_src_file = otter_string_format(
      allocator, "%s/parser_integration_tests.c", config->src_dir);
  if (parser_integration_tests_src_file == NULL) {
    return;
  }

  OTTER_CLEANUP(otter_target_free_p)
  otter_target *allocator_obj = otter_target_create_c_object(
      otter_string_cstr(allocator_o_file), cc_flags, CC_INCLUDE_FLAGS,
      allocator, filesystem, logger, otter_string_cstr(allocator_src_file),
      NULL);
  if (allocator_obj == NULL) {
    return;
  }

  OTTER_CLEANUP(otter_target_free_p)
  otter_target *string_obj = otter_target_create_c_object(
      otter_string_cstr(string_o_file), cc_flags, CC_INCLUDE_FLAGS, allocator,
      filesystem, logger, otter_string_cstr(string_src_file), NULL);
  if (string_obj == NULL) {
    return;
  }

  otter_target_add_dependency(string_obj, allocator_obj);

  OTTER_CLEANUP(otter_target_free_p)
  otter_target *array_obj = otter_target_create_c_object(
      otter_string_cstr(array_o_file), cc_flags, CC_INCLUDE_FLAGS, allocator,
      filesystem, logger, otter_string_cstr(array_src_file), NULL);
  if (array_obj == NULL) {
    return;
  }

  otter_target_add_dependency(array_obj, allocator_obj);

  OTTER_CLEANUP(otter_target_free_p)
  otter_target *cstring_obj = otter_target_create_c_object(
      otter_string_cstr(cstring_o_file), cc_flags, CC_INCLUDE_FLAGS, allocator,
      filesystem, logger, otter_string_cstr(cstring_src_file), NULL);
  if (cstring_obj == NULL) {
    return;
  }

  otter_target_add_dependency(cstring_obj, allocator_obj);

  OTTER_CLEANUP(otter_target_free_p)
  otter_target *logger_obj = otter_target_create_c_object(
      otter_string_cstr(logger_o_file), cc_flags, CC_INCLUDE_FLAGS, allocator,
      filesystem, logger, otter_string_cstr(logger_src_file), NULL);
  if (logger_obj == NULL) {
    return;
  }

  otter_target_add_dependency(logger_obj, cstring_obj);
  otter_target_add_dependency(logger_obj, array_obj);
  otter_target_add_dependency(logger_obj, allocator_obj);

  OTTER_CLEANUP(otter_target_free_p)
  otter_target *file_obj = otter_target_create_c_object(
      otter_string_cstr(file_o_file), cc_flags, CC_INCLUDE_FLAGS, allocator,
      filesystem, logger, otter_string_cstr(file_src_file), NULL);
  if (file_obj == NULL) {
    return;
  }

  OTTER_CLEANUP(otter_target_free_p)
  otter_target *filesystem_obj = otter_target_create_c_object(
      otter_string_cstr(filesystem_o_file), cc_flags, CC_INCLUDE_FLAGS,
      allocator, filesystem, logger, otter_string_cstr(filesystem_src_file),
      NULL);
  if (filesystem_obj == NULL) {
    return;
  }

  otter_target_add_dependency(filesystem_obj, file_obj);
  otter_target_add_dependency(filesystem_obj, allocator_obj);

  OTTER_CLEANUP(otter_target_free_p)
  otter_target *target_obj = otter_target_create_c_object(
      otter_string_cstr(target_o_file), cc_flags, CC_INCLUDE_FLAGS, allocator,
      filesystem, logger, otter_string_cstr(target_src_file), NULL);
  if (target_obj == NULL) {
    return;
  }

  otter_target_add_dependency(target_obj, allocator_obj);
  otter_target_add_dependency(target_obj, array_obj);
  otter_target_add_dependency(target_obj, filesystem_obj);
  otter_target_add_dependency(target_obj, logger_obj);
  otter_target_add_dependency(target_obj, string_obj);

  OTTER_CLEANUP(otter_target_free_p)
  otter_target *token_obj = otter_target_create_c_object(
      otter_string_cstr(token_o_file), cc_flags, CC_INCLUDE_FLAGS, allocator,
      filesystem, logger, otter_string_cstr(token_src_file), NULL);
  if (token_obj == NULL) {
    return;
  }

  otter_target_add_dependency(token_obj, allocator_obj);

  OTTER_CLEANUP(otter_target_free_p)
  otter_target *node_obj = otter_target_create_c_object(
      otter_string_cstr(node_o_file), cc_flags, CC_INCLUDE_FLAGS, allocator,
      filesystem, logger, otter_string_cstr(node_src_file), NULL);
  if (node_obj == NULL) {
    return;
  }

  otter_target_add_dependency(node_obj, allocator_obj);
  otter_target_add_dependency(node_obj, array_obj);

  OTTER_CLEANUP(otter_target_free_p)
  otter_target *lexer_obj = otter_target_create_c_object(
      otter_string_cstr(lexer_o_file), cc_flags, CC_INCLUDE_FLAGS, allocator,
      filesystem, logger, otter_string_cstr(lexer_src_file), NULL);
  if (lexer_obj == NULL) {
    return;
  }

  otter_target_add_dependency(lexer_obj, array_obj);
  otter_target_add_dependency(lexer_obj, cstring_obj);

  OTTER_CLEANUP(otter_target_free_p)
  otter_target *parser_obj = otter_target_create_c_object(
      otter_string_cstr(parser_o_file), cc_flags, CC_INCLUDE_FLAGS, allocator,
      filesystem, logger, otter_string_cstr(parser_src_file), NULL);
  if (parser_obj == NULL) {
    return;
  }

  otter_target_add_dependency(parser_obj, allocator_obj);
  otter_target_add_dependency(parser_obj, logger_obj);
  otter_target_add_dependency(parser_obj, node_obj);
  otter_target_add_dependency(parser_obj, cstring_obj);
  otter_target_add_dependency(parser_obj, token_obj);

  OTTER_CLEANUP(otter_target_free_p)
  otter_target *bytecode_obj = otter_target_create_c_object(
      otter_string_cstr(bytecode_o_file), cc_flags, CC_INCLUDE_FLAGS, allocator,
      filesystem, logger, otter_string_cstr(bytecode_src_file), NULL);
  if (bytecode_obj == NULL) {
    return;
  }

  OTTER_CLEANUP(otter_target_free_p)
  otter_target *vm_obj = otter_target_create_c_object(
      otter_string_cstr(vm_o_file), cc_flags, CC_INCLUDE_FLAGS, allocator,
      filesystem, logger, otter_string_cstr(vm_src_file), NULL);
  if (vm_obj == NULL) {
    return;
  }

  otter_target_add_dependency(vm_obj, allocator_obj);
  otter_target_add_dependency(vm_obj, logger_obj);
  otter_target_add_dependency(vm_obj, bytecode_obj);

  /* Main executable */
  OTTER_CLEANUP(otter_string_free_p)
  otter_string *exe_name = otter_string_format(allocator, "%s/otter%s",
                                               config->out_dir, config->suffix);
  if (exe_name == NULL) {
    return;
  }

  OTTER_CLEANUP(otter_string_free_p)
  otter_string *exe_flags =
      otter_string_format(allocator, "%s%s", cc_flags, ll_flags);
  if (exe_flags == NULL) {
    return;
  }

  OTTER_CLEANUP(otter_target_free_p)
  otter_target *exe = otter_target_create_c_executable(
      otter_string_cstr(exe_name), otter_string_cstr(exe_flags),
      CC_INCLUDE_FLAGS, allocator, filesystem, logger,
      (const char *[]){otter_string_cstr(main_src_file), NULL},
      (otter_target *[]){vm_obj, NULL});
  if (exe == NULL) {
    return;
  }

  otter_target_execute(exe);

  /* Test driver */
  OTTER_CLEANUP(otter_target_free_p)
  otter_target *test_obj = otter_target_create_c_object(
      otter_string_cstr(test_o_file), cc_flags, CC_INCLUDE_FLAGS, allocator,
      filesystem, logger, otter_string_cstr(test_src_file), NULL);
  if (test_obj == NULL) {
    return;
  }

  otter_target_add_dependency(test_obj, allocator_obj);

  OTTER_CLEANUP(otter_string_free_p)
  otter_string *test_exe_name = otter_string_format(
      allocator, "%s/test%s", config->out_dir, config->suffix);
  if (test_exe_name == NULL) {
    return;
  }

  OTTER_CLEANUP(otter_target_free_p)
  otter_target *test_driver = otter_target_create_c_executable(
      otter_string_cstr(test_exe_name), otter_string_cstr(exe_flags),
      CC_INCLUDE_FLAGS, allocator, filesystem, logger,
      (const char *[]){otter_string_cstr(test_driver_src_file), NULL},
      (otter_target *[]){allocator_obj, NULL});
  if (test_driver == NULL) {
    return;
  }

  otter_target_execute(test_driver);

  /* Test shared objects */
  OTTER_CLEANUP(otter_string_free_p)
  otter_string *cstring_tests_name = otter_string_format(
      allocator, "%s/cstring_tests%s.so", config->out_dir, config->suffix);
  if (cstring_tests_name == NULL) {
    return;
  }

  OTTER_CLEANUP(otter_target_free_p)
  otter_target *cstring_tests = otter_target_create_c_shared_object(
      otter_string_cstr(cstring_tests_name), cc_flags, CC_INCLUDE_FLAGS,
      allocator, filesystem, logger,
      (const char *[]){otter_string_cstr(cstring_tests_src_file), NULL},
      (otter_target *[]){test_obj, cstring_obj, NULL});
  if (cstring_tests == NULL) {
    return;
  }

  otter_target_execute(cstring_tests);

  OTTER_CLEANUP(otter_string_free_p)
  otter_string *string_tests_name = otter_string_format(
      allocator, "%s/string_tests%s.so", config->out_dir, config->suffix);
  if (string_tests_name == NULL) {
    return;
  }

  OTTER_CLEANUP(otter_target_free_p)
  otter_target *string_tests = otter_target_create_c_shared_object(
      otter_string_cstr(string_tests_name), otter_string_cstr(exe_flags),
      CC_INCLUDE_FLAGS, allocator, filesystem, logger,
      (const char *[]){otter_string_cstr(string_tests_src_file), NULL},
      (otter_target *[]){test_obj, string_obj, NULL});
  if (string_tests == NULL) {
    return;
  }

  otter_target_execute(string_tests);

  OTTER_CLEANUP(otter_string_free_p)
  otter_string *array_tests_name = otter_string_format(
      allocator, "%s/array_tests%s.so", config->out_dir, config->suffix);
  if (array_tests_name == NULL) {
    return;
  }

  OTTER_CLEANUP(otter_target_free_p)
  otter_target *array_tests = otter_target_create_c_shared_object(
      otter_string_cstr(array_tests_name), otter_string_cstr(exe_flags),
      CC_INCLUDE_FLAGS, allocator, filesystem, logger,
      (const char *[]){otter_string_cstr(array_tests_src_file), NULL},
      (otter_target *[]){test_obj, array_obj, NULL});
  if (array_tests == NULL) {
    return;
  }

  otter_target_execute(array_tests);

  OTTER_CLEANUP(otter_string_free_p)
  otter_string *lexer_tests_name = otter_string_format(
      allocator, "%s/lexer_tests%s.so", config->out_dir, config->suffix);
  if (lexer_tests_name == NULL) {
    return;
  }

  OTTER_CLEANUP(otter_target_free_p)
  otter_target *lexer_tests = otter_target_create_c_shared_object(
      otter_string_cstr(lexer_tests_name), otter_string_cstr(exe_flags),
      CC_INCLUDE_FLAGS, allocator, filesystem, logger,
      (const char *[]){otter_string_cstr(lexer_tests_src_file), NULL},
      (otter_target *[]){test_obj, lexer_obj, token_obj, NULL});
  if (lexer_tests == NULL) {
    return;
  }

  otter_target_execute(lexer_tests);

  OTTER_CLEANUP(otter_string_free_p)
  otter_string *parser_tests_name = otter_string_format(
      allocator, "%s/parser_tests%s.so", config->out_dir, config->suffix);
  if (parser_tests_name == NULL) {
    return;
  }

  OTTER_CLEANUP(otter_target_free_p)
  otter_target *parser_tests = otter_target_create_c_shared_object(
      otter_string_cstr(parser_tests_name), otter_string_cstr(exe_flags),
      CC_INCLUDE_FLAGS, allocator, filesystem, logger,
      (const char *[]){otter_string_cstr(parser_tests_src_file), NULL},
      (otter_target *[]){test_obj, cstring_obj, node_obj, parser_obj, NULL});
  if (parser_tests == NULL) {
    return;
  }

  otter_target_execute(parser_tests);

  OTTER_CLEANUP(otter_string_free_p)
  otter_string *parser_int_tests_name =
      otter_string_format(allocator, "%s/parser_integration_tests%s.so",
                          config->out_dir, config->suffix);
  if (parser_int_tests_name == NULL) {
    return;
  }

  OTTER_CLEANUP(otter_target_free_p)
  otter_target *parser_integration_tests = otter_target_create_c_shared_object(
      otter_string_cstr(parser_int_tests_name), otter_string_cstr(exe_flags),
      CC_INCLUDE_FLAGS, allocator, filesystem, logger,
      (const char *[]){otter_string_cstr(parser_integration_tests_src_file),
                       NULL},
      (otter_target *[]){test_obj, lexer_obj, node_obj, parser_obj, NULL});
  if (parser_integration_tests == NULL) {
    return;
  }

  otter_target_execute(parser_integration_tests);
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
  build_config bootstrap_config = {"", CC_FLAGS_RELEASE, LL_FLAGS_RELEASE,
                                   "./src", "./release"};

  OTTER_CLEANUP(otter_string_free_p)
  otter_string *allocator_src_file = otter_string_format(
      allocator, "%s/allocator.c", bootstrap_config.src_dir);
  if (allocator_src_file == NULL) {
    return 1;
  }

  OTTER_CLEANUP(otter_string_free_p)
  otter_string *allocator_o_file = otter_string_format(
      allocator, "%s/allocator.o", bootstrap_config.out_dir);
  if (allocator_o_file == NULL) {
    return 1;
  }

  OTTER_CLEANUP(otter_string_free_p)
  otter_string *string_src_file =
      otter_string_format(allocator, "%s/string.c", bootstrap_config.src_dir);
  if (string_src_file == NULL) {
    return 1;
  }

  OTTER_CLEANUP(otter_string_free_p)
  otter_string *string_o_file =
      otter_string_format(allocator, "%s/string.o", bootstrap_config.out_dir);
  if (string_o_file == NULL) {
    return 1;
  }

  OTTER_CLEANUP(otter_string_free_p)
  otter_string *array_src_file =
      otter_string_format(allocator, "%s/array.c", bootstrap_config.src_dir);
  if (array_src_file == NULL) {
    return 1;
  }

  OTTER_CLEANUP(otter_string_free_p)
  otter_string *array_o_file =
      otter_string_format(allocator, "%s/array.o", bootstrap_config.out_dir);
  if (array_o_file == NULL) {
    return 1;
  }

  OTTER_CLEANUP(otter_string_free_p)
  otter_string *cstring_src_file =
      otter_string_format(allocator, "%s/cstring.c", bootstrap_config.src_dir);
  if (cstring_src_file == NULL) {
    return 1;
  }

  OTTER_CLEANUP(otter_string_free_p)
  otter_string *cstring_o_file =
      otter_string_format(allocator, "%s/cstring.o", bootstrap_config.out_dir);
  if (cstring_o_file == NULL) {
    return 1;
  }

  OTTER_CLEANUP(otter_string_free_p)
  otter_string *logger_src_file =
      otter_string_format(allocator, "%s/logger.c", bootstrap_config.src_dir);
  if (logger_src_file == NULL) {
    return 1;
  }

  OTTER_CLEANUP(otter_string_free_p)
  otter_string *logger_o_file =
      otter_string_format(allocator, "%s/logger.o", bootstrap_config.out_dir);
  if (logger_o_file == NULL) {
    return 1;
  }

  OTTER_CLEANUP(otter_string_free_p)
  otter_string *file_src_file =
      otter_string_format(allocator, "%s/file.c", bootstrap_config.src_dir);
  if (file_src_file == NULL) {
    return 1;
  }

  OTTER_CLEANUP(otter_string_free_p)
  otter_string *file_o_file =
      otter_string_format(allocator, "%s/file.o", bootstrap_config.out_dir);
  if (file_o_file == NULL) {
    return 1;
  }

  OTTER_CLEANUP(otter_string_free_p)
  otter_string *filesystem_src_file = otter_string_format(
      allocator, "%s/filesystem.c", bootstrap_config.src_dir);
  if (filesystem_src_file == NULL) {
    return 1;
  }

  OTTER_CLEANUP(otter_string_free_p)
  otter_string *filesystem_o_file = otter_string_format(
      allocator, "%s/filesystem.o", bootstrap_config.out_dir);
  if (filesystem_o_file == NULL) {
    return 1;
  }

  OTTER_CLEANUP(otter_string_free_p)
  otter_string *target_src_file =
      otter_string_format(allocator, "%s/target.c", bootstrap_config.src_dir);
  if (target_src_file == NULL) {
    return 1;
  }

  OTTER_CLEANUP(otter_string_free_p)
  otter_string *target_o_file =
      otter_string_format(allocator, "%s/target.o", bootstrap_config.out_dir);
  if (target_o_file == NULL) {
    return 1;
  }

  OTTER_CLEANUP(otter_string_free_p)
  otter_string *make_src_file =
      otter_string_format(allocator, "%s/make.c", bootstrap_config.src_dir);
  if (make_src_file == NULL) {
    return 1;
  }

  OTTER_CLEANUP(otter_string_free_p)
  otter_string *make_o_file = otter_string_from_cstr(allocator, "otter_make");
  if (make_o_file == NULL) {
    return 1;
  }

  OTTER_CLEANUP(otter_target_free_p)
  otter_target *allocator_obj = otter_target_create_c_object(
      otter_string_cstr(allocator_o_file), CC_FLAGS_RELEASE, CC_INCLUDE_FLAGS,
      allocator, filesystem, logger, otter_string_cstr(allocator_src_file),
      NULL);
  if (allocator_obj == NULL) {
    return 1;
  }

  OTTER_CLEANUP(otter_target_free_p)
  otter_target *string_obj = otter_target_create_c_object(
      otter_string_cstr(string_o_file), CC_FLAGS_RELEASE, CC_INCLUDE_FLAGS,
      allocator, filesystem, logger, otter_string_cstr(string_src_file), NULL);
  if (string_obj == NULL) {
    return 1;
  }

  otter_target_add_dependency(string_obj, allocator_obj);

  OTTER_CLEANUP(otter_target_free_p)
  otter_target *array_obj = otter_target_create_c_object(
      otter_string_cstr(array_o_file), CC_FLAGS_RELEASE, CC_INCLUDE_FLAGS,
      allocator, filesystem, logger, otter_string_cstr(array_src_file), NULL);
  if (array_obj == NULL) {
    return 1;
  }

  otter_target_add_dependency(array_obj, allocator_obj);

  OTTER_CLEANUP(otter_target_free_p)
  otter_target *cstring_obj = otter_target_create_c_object(
      otter_string_cstr(cstring_o_file), CC_FLAGS_RELEASE, CC_INCLUDE_FLAGS,
      allocator, filesystem, logger, otter_string_cstr(cstring_src_file), NULL);
  if (cstring_obj == NULL) {
    return 1;
  }

  otter_target_add_dependency(cstring_obj, allocator_obj);

  OTTER_CLEANUP(otter_target_free_p)
  otter_target *logger_obj = otter_target_create_c_object(
      otter_string_cstr(logger_o_file), CC_FLAGS_RELEASE, CC_INCLUDE_FLAGS,
      allocator, filesystem, logger, otter_string_cstr(logger_src_file), NULL);
  if (logger_obj == NULL) {
    return 1;
  }

  otter_target_add_dependency(logger_obj, cstring_obj);
  otter_target_add_dependency(logger_obj, array_obj);
  otter_target_add_dependency(logger_obj, allocator_obj);

  OTTER_CLEANUP(otter_target_free_p)
  otter_target *file_obj = otter_target_create_c_object(
      otter_string_cstr(file_o_file), CC_FLAGS_RELEASE, CC_INCLUDE_FLAGS,
      allocator, filesystem, logger, otter_string_cstr(file_src_file), NULL);
  if (file_obj == NULL) {
    return 1;
  }

  OTTER_CLEANUP(otter_target_free_p)
  otter_target *filesystem_obj = otter_target_create_c_object(
      otter_string_cstr(filesystem_o_file), CC_FLAGS_RELEASE, CC_INCLUDE_FLAGS,
      allocator, filesystem, logger, otter_string_cstr(filesystem_src_file),
      NULL);
  if (filesystem_obj == NULL) {
    return 1;
  }

  otter_target_add_dependency(filesystem_obj, file_obj);
  otter_target_add_dependency(filesystem_obj, allocator_obj);

  OTTER_CLEANUP(otter_target_free_p)
  otter_target *target_obj = otter_target_create_c_object(
      otter_string_cstr(target_o_file), CC_FLAGS_RELEASE, CC_INCLUDE_FLAGS,
      allocator, filesystem, logger, otter_string_cstr(target_src_file), NULL);
  if (target_obj == NULL) {
    return 1;
  }
  otter_target_add_dependency(target_obj, allocator_obj);
  otter_target_add_dependency(target_obj, array_obj);
  otter_target_add_dependency(target_obj, filesystem_obj);
  otter_target_add_dependency(target_obj, logger_obj);
  otter_target_add_dependency(target_obj, string_obj);

  OTTER_CLEANUP(otter_string_free_p)
  otter_string *make_flags =
      otter_string_format(allocator, "%s -lgnutls", CC_FLAGS_RELEASE);
  if (make_flags == NULL) {
    return 1;
  }

  OTTER_CLEANUP(otter_target_free_p)
  otter_target *make_exe = otter_target_create_c_executable(
      otter_string_cstr(make_o_file), otter_string_cstr(make_flags),
      CC_INCLUDE_FLAGS, allocator, filesystem, logger,
      (const char *[]){otter_string_cstr(make_src_file), NULL},
      (otter_target *[]){allocator_obj, filesystem_obj, logger_obj, target_obj,
                         NULL});
  if (make_exe == NULL) {
    return 1;
  }

  otter_target_execute(make_exe);

  if (mode == BUILD_MODE_DEBUG) {
    build_config configs[] = {
        {"", CC_FLAGS_DEBUG, LL_FLAGS_DEBUG, "./src", "./debug"},
        {"_coverage", CC_FLAGS_COVERAGE, LL_FLAGS_COVERAGE, "./src", "./debug"},
    };

    for (size_t i = 0; i < sizeof(configs) / sizeof(configs[0]); i++) {
      build_program_and_tests(allocator, filesystem, logger, &configs[i]);
    }
  } else {
    /* Release mode: only build the optimized version */
    build_config config = {"", CC_FLAGS_RELEASE, LL_FLAGS_RELEASE, "./src",
                           "./release"};
    build_program_and_tests(allocator, filesystem, logger, &config);
  }

  return 0;
}
