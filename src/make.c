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
  char *allocator_o_file = NULL;
  char *string_o_file = NULL;
  char *array_o_file = NULL;
  char *cstring_o_file = NULL;
  char *logger_o_file = NULL;
  char *file_o_file = NULL;
  char *filesystem_o_file = NULL;
  char *target_o_file = NULL;
  char *token_o_file = NULL;
  char *node_o_file = NULL;
  char *lexer_o_file = NULL;
  char *parser_o_file = NULL;
  char *bytecode_o_file = NULL;
  char *vm_o_file = NULL;
  char *test_o_file = NULL;
  char *allocator_src_file = NULL;
  char *string_src_file = NULL;
  char *array_src_file = NULL;
  char *cstring_src_file = NULL;
  char *logger_src_file = NULL;
  char *file_src_file = NULL;
  char *filesystem_src_file = NULL;
  char *target_src_file = NULL;
  char *token_src_file = NULL;
  char *node_src_file = NULL;
  char *lexer_src_file = NULL;
  char *parser_src_file = NULL;
  char *bytecode_src_file = NULL;
  char *vm_src_file = NULL;
  char *main_src_file = NULL;
  char *test_src_file = NULL;
  char *test_driver_src_file = NULL;
  char *cstring_tests_src_file = NULL;
  char *string_tests_src_file = NULL;
  char *array_tests_src_file = NULL;
  char *lexer_tests_src_file = NULL;
  char *parser_tests_src_file = NULL;
  char *parser_integration_tests_src_file = NULL;
  char *exe_name = NULL;
  char *exe_flags = NULL;
  char *test_exe_name = NULL;
  char *cstring_tests_name = NULL;
  char *string_tests_name = NULL;
  char *array_tests_name = NULL;
  char *lexer_tests_name = NULL;
  char *parser_tests_name = NULL;
  char *parser_int_tests_name = NULL;
  OTTER_CLEANUP(otter_target_free_p) otter_target *allocator_obj = NULL;
  OTTER_CLEANUP(otter_target_free_p) otter_target *string_obj = NULL;
  OTTER_CLEANUP(otter_target_free_p) otter_target *array_obj = NULL;
  OTTER_CLEANUP(otter_target_free_p) otter_target *cstring_obj = NULL;
  OTTER_CLEANUP(otter_target_free_p) otter_target *logger_obj = NULL;
  OTTER_CLEANUP(otter_target_free_p) otter_target *file_obj = NULL;
  OTTER_CLEANUP(otter_target_free_p) otter_target *filesystem_obj = NULL;
  OTTER_CLEANUP(otter_target_free_p) otter_target *target_obj = NULL;
  OTTER_CLEANUP(otter_target_free_p) otter_target *token_obj = NULL;
  OTTER_CLEANUP(otter_target_free_p) otter_target *node_obj = NULL;
  OTTER_CLEANUP(otter_target_free_p)
  otter_target *lexer_obj = NULL;
  OTTER_CLEANUP(otter_target_free_p) otter_target *parser_obj = NULL;
  OTTER_CLEANUP(otter_target_free_p) otter_target *bytecode_obj = NULL;
  OTTER_CLEANUP(otter_target_free_p) otter_target *vm_obj = NULL;
  OTTER_CLEANUP(otter_target_free_p) otter_target *exe = NULL;
  OTTER_CLEANUP(otter_target_free_p) otter_target *test_obj = NULL;
  OTTER_CLEANUP(otter_target_free_p) otter_target *test_driver = NULL;
  OTTER_CLEANUP(otter_target_free_p) otter_target *cstring_tests = NULL;
  OTTER_CLEANUP(otter_target_free_p) otter_target *string_tests = NULL;
  OTTER_CLEANUP(otter_target_free_p) otter_target *array_tests = NULL;
  OTTER_CLEANUP(otter_target_free_p) otter_target *lexer_tests = NULL;
  OTTER_CLEANUP(otter_target_free_p) otter_target *parser_tests = NULL;
  OTTER_CLEANUP(otter_target_free_p)
  otter_target *parser_integration_tests = NULL;

  if (!otter_asprintf(allocator, &allocator_o_file, "%s/allocator%s.o",
                      config->out_dir, config->suffix)) {
    goto cleanup;
  }

  if (!otter_asprintf(allocator, &string_o_file, "%s/string%s.o",
                      config->out_dir, config->suffix)) {
    goto cleanup;
  }

  if (!otter_asprintf(allocator, &array_o_file, "%s/array%s.o", config->out_dir,
                      config->suffix)) {
    goto cleanup;
  }

  if (!otter_asprintf(allocator, &cstring_o_file, "%s/cstring%s.o",
                      config->out_dir, config->suffix)) {
    goto cleanup;
  }

  if (!otter_asprintf(allocator, &logger_o_file, "%s/logger%s.o",
                      config->out_dir, config->suffix)) {
    goto cleanup;
  }

  if (!otter_asprintf(allocator, &file_o_file, "%s/file%s.o", config->out_dir,
                      config->suffix)) {
    goto cleanup;
  }

  if (!otter_asprintf(allocator, &filesystem_o_file, "%s/filesystem%s.o",
                      config->out_dir, config->suffix)) {
    goto cleanup;
  }

  if (!otter_asprintf(allocator, &target_o_file, "%s/target%s.o",
                      config->out_dir, config->suffix)) {
    goto cleanup;
  }

  if (!otter_asprintf(allocator, &token_o_file, "%s/token%s.o", config->out_dir,
                      config->suffix)) {
    goto cleanup;
  }

  if (!otter_asprintf(allocator, &node_o_file, "%s/node%s.o", config->out_dir,
                      config->suffix)) {
    goto cleanup;
  }

  if (!otter_asprintf(allocator, &lexer_o_file, "%s/lexer%s.o", config->out_dir,
                      config->suffix)) {
    goto cleanup;
  }

  if (!otter_asprintf(allocator, &parser_o_file, "%s/parser%s.o",
                      config->out_dir, config->suffix)) {
    goto cleanup;
  }

  if (!otter_asprintf(allocator, &bytecode_o_file, "%s/bytecode%s.o",
                      config->out_dir, config->suffix)) {
    goto cleanup;
  }

  if (!otter_asprintf(allocator, &vm_o_file, "%s/vm%s.o", config->out_dir,
                      config->suffix)) {
    goto cleanup;
  }

  if (!otter_asprintf(allocator, &test_o_file, "%s/test%s.o", config->out_dir,
                      config->suffix)) {
    goto cleanup;
  }

  if (!otter_asprintf(allocator, &allocator_src_file, "%s/allocator.c",
                      config->src_dir)) {
    goto cleanup;
  }

  if (!otter_asprintf(allocator, &string_src_file, "%s/string.c",
                      config->src_dir)) {
    goto cleanup;
  }

  if (!otter_asprintf(allocator, &array_src_file, "%s/array.c",
                      config->src_dir)) {
    goto cleanup;
  }

  if (!otter_asprintf(allocator, &cstring_src_file, "%s/cstring.c",
                      config->src_dir)) {
    goto cleanup;
  }

  if (!otter_asprintf(allocator, &logger_src_file, "%s/logger.c",
                      config->src_dir)) {
    goto cleanup;
  }

  if (!otter_asprintf(allocator, &file_src_file, "%s/file.c",
                      config->src_dir)) {
    goto cleanup;
  }

  if (!otter_asprintf(allocator, &filesystem_src_file, "%s/filesystem.c",
                      config->src_dir)) {
    goto cleanup;
  }

  if (!otter_asprintf(allocator, &target_src_file, "%s/target.c",
                      config->src_dir)) {
    goto cleanup;
  }

  if (!otter_asprintf(allocator, &token_src_file, "%s/token.c",
                      config->src_dir)) {
    goto cleanup;
  }

  if (!otter_asprintf(allocator, &node_src_file, "%s/node.c",
                      config->src_dir)) {
    goto cleanup;
  }

  if (!otter_asprintf(allocator, &lexer_src_file, "%s/lexer.c",
                      config->src_dir)) {
    goto cleanup;
  }

  if (!otter_asprintf(allocator, &parser_src_file, "%s/parser.c",
                      config->src_dir)) {
    goto cleanup;
  }

  if (!otter_asprintf(allocator, &bytecode_src_file, "%s/bytecode.c",
                      config->src_dir)) {
    goto cleanup;
  }

  if (!otter_asprintf(allocator, &vm_src_file, "%s/vm.c", config->src_dir)) {
    goto cleanup;
  }

  if (!otter_asprintf(allocator, &main_src_file, "%s/otter.c",
                      config->src_dir)) {
    goto cleanup;
  }

  if (!otter_asprintf(allocator, &test_src_file, "%s/test.c",
                      config->src_dir)) {
    goto cleanup;
  }

  if (!otter_asprintf(allocator, &test_driver_src_file, "%s/test_driver.c",
                      config->src_dir)) {
    goto cleanup;
  }

  if (!otter_asprintf(allocator, &cstring_tests_src_file, "%s/cstring_tests.c",
                      config->src_dir)) {
    goto cleanup;
  }

  if (!otter_asprintf(allocator, &string_tests_src_file, "%s/string_tests.c",
                      config->src_dir)) {
    goto cleanup;
  }

  if (!otter_asprintf(allocator, &array_tests_src_file, "%s/array_tests.c",
                      config->src_dir)) {
    goto cleanup;
  }

  if (!otter_asprintf(allocator, &lexer_tests_src_file, "%s/lexer_tests.c",
                      config->src_dir)) {
    goto cleanup;
  }

  if (!otter_asprintf(allocator, &parser_tests_src_file, "%s/parser_tests.c",
                      config->src_dir)) {
    goto cleanup;
  }

  if (!otter_asprintf(allocator, &parser_integration_tests_src_file,
                      "%s/parser_integration_tests.c", config->src_dir)) {
    goto cleanup;
  }

  allocator_obj = otter_target_create_c_object(
      allocator_o_file, cc_flags, CC_INCLUDE_FLAGS, allocator, filesystem,
      logger, allocator_src_file, NULL);
  if (allocator_obj == NULL) {
    goto cleanup;
  }

  string_obj = otter_target_create_c_object(
      string_o_file, cc_flags, CC_INCLUDE_FLAGS, allocator, filesystem, logger,
      string_src_file, NULL);
  if (string_obj == NULL) {
    goto cleanup;
  }

  otter_target_add_dependency(string_obj, allocator_obj);

  array_obj = otter_target_create_c_object(
      array_o_file, cc_flags, CC_INCLUDE_FLAGS, allocator, filesystem, logger,
      array_src_file, NULL);
  if (array_obj == NULL) {
    goto cleanup;
  }

  otter_target_add_dependency(array_obj, allocator_obj);

  cstring_obj = otter_target_create_c_object(
      cstring_o_file, cc_flags, CC_INCLUDE_FLAGS, allocator, filesystem, logger,
      cstring_src_file, NULL);
  if (cstring_obj == NULL) {
    goto cleanup;
  }

  otter_target_add_dependency(cstring_obj, allocator_obj);

  logger_obj = otter_target_create_c_object(
      logger_o_file, cc_flags, CC_INCLUDE_FLAGS, allocator, filesystem, logger,
      logger_src_file, NULL);
  if (logger_obj == NULL) {
    goto cleanup;
  }

  otter_target_add_dependency(logger_obj, cstring_obj);
  otter_target_add_dependency(logger_obj, array_obj);
  otter_target_add_dependency(logger_obj, allocator_obj);

  file_obj = otter_target_create_c_object(
      file_o_file, cc_flags, CC_INCLUDE_FLAGS, allocator, filesystem, logger,
      file_src_file, NULL);
  if (file_obj == NULL) {
    goto cleanup;
  }

  filesystem_obj = otter_target_create_c_object(
      filesystem_o_file, cc_flags, CC_INCLUDE_FLAGS, allocator, filesystem,
      logger, filesystem_src_file, NULL);
  if (filesystem_obj == NULL) {
    goto cleanup;
  }

  otter_target_add_dependency(filesystem_obj, file_obj);
  otter_target_add_dependency(filesystem_obj, allocator_obj);

  target_obj = otter_target_create_c_object(
      target_o_file, cc_flags, CC_INCLUDE_FLAGS, allocator, filesystem, logger,
      target_src_file, NULL);
  if (target_obj == NULL) {
    goto cleanup;
  }

  otter_target_add_dependency(target_obj, allocator_obj);
  otter_target_add_dependency(target_obj, array_obj);
  otter_target_add_dependency(target_obj, filesystem_obj);
  otter_target_add_dependency(target_obj, logger_obj);
  otter_target_add_dependency(target_obj, string_obj);

  token_obj = otter_target_create_c_object(
      token_o_file, cc_flags, CC_INCLUDE_FLAGS, allocator, filesystem, logger,
      token_src_file, NULL);
  if (token_obj == NULL) {
    goto cleanup;
  }

  otter_target_add_dependency(token_obj, allocator_obj);

  node_obj = otter_target_create_c_object(
      node_o_file, cc_flags, CC_INCLUDE_FLAGS, allocator, filesystem, logger,
      node_src_file, NULL);
  if (node_obj == NULL) {
    goto cleanup;
  }

  otter_target_add_dependency(node_obj, allocator_obj);
  otter_target_add_dependency(node_obj, array_obj);

  lexer_obj = otter_target_create_c_object(
      lexer_o_file, cc_flags, CC_INCLUDE_FLAGS, allocator, filesystem, logger,
      lexer_src_file, NULL);
  if (lexer_obj == NULL) {
    goto cleanup;
  }

  otter_target_add_dependency(lexer_obj, array_obj);
  otter_target_add_dependency(lexer_obj, cstring_obj);

  parser_obj = otter_target_create_c_object(
      parser_o_file, cc_flags, CC_INCLUDE_FLAGS, allocator, filesystem, logger,
      parser_src_file, NULL);
  if (parser_obj == NULL) {
    goto cleanup;
  }

  otter_target_add_dependency(parser_obj, allocator_obj);
  otter_target_add_dependency(parser_obj, logger_obj);
  otter_target_add_dependency(parser_obj, node_obj);
  otter_target_add_dependency(parser_obj, cstring_obj);
  otter_target_add_dependency(parser_obj, token_obj);

  bytecode_obj = otter_target_create_c_object(
      bytecode_o_file, cc_flags, CC_INCLUDE_FLAGS, allocator, filesystem,
      logger, bytecode_src_file, NULL);
  if (bytecode_obj == NULL) {
    goto cleanup;
  }

  vm_obj = otter_target_create_c_object(vm_o_file, cc_flags, CC_INCLUDE_FLAGS,
                                        allocator, filesystem, logger,
                                        vm_src_file, NULL);
  if (vm_obj == NULL) {
    goto cleanup;
  }

  otter_target_add_dependency(vm_obj, allocator_obj);
  otter_target_add_dependency(vm_obj, logger_obj);
  otter_target_add_dependency(vm_obj, bytecode_obj);

  /* Main executable */
  if (!otter_asprintf(allocator, &exe_name, "%s/otter%s", config->out_dir,
                      config->suffix)) {
    goto cleanup;
  }

  if (!otter_asprintf(allocator, &exe_flags, "%s%s", cc_flags, ll_flags)) {
    goto cleanup;
  }

  exe = otter_target_create_c_executable(
      exe_name, exe_flags, CC_INCLUDE_FLAGS, allocator, filesystem, logger,
      (const char *[]){main_src_file, NULL}, (otter_target *[]){vm_obj, NULL});
  if (exe == NULL) {
    goto cleanup;
  }

  otter_target_execute(exe);

  /* Test driver */
  test_obj = otter_target_create_c_object(
      test_o_file, cc_flags, CC_INCLUDE_FLAGS, allocator, filesystem, logger,
      test_src_file, NULL);
  if (test_obj == NULL) {
    goto cleanup;
  }

  otter_target_add_dependency(test_obj, allocator_obj);

  if (!otter_asprintf(allocator, &test_exe_name, "%s/test%s", config->out_dir,
                      config->suffix)) {
    goto cleanup;
  }

  test_driver = otter_target_create_c_executable(
      test_exe_name, exe_flags, CC_INCLUDE_FLAGS, allocator, filesystem, logger,
      (const char *[]){test_driver_src_file, NULL},
      (otter_target *[]){allocator_obj, NULL});
  if (test_driver == NULL) {
    goto cleanup;
  }

  otter_target_execute(test_driver);

  /* Test shared objects */
  if (!otter_asprintf(allocator, &cstring_tests_name, "%s/cstring_tests%s.so",
                      config->out_dir, config->suffix)) {
    goto cleanup;
  }

  cstring_tests = otter_target_create_c_shared_object(
      cstring_tests_name, cc_flags, CC_INCLUDE_FLAGS, allocator, filesystem,
      logger, (const char *[]){cstring_tests_src_file, NULL},
      (otter_target *[]){test_obj, cstring_obj, NULL});
  if (cstring_tests == NULL) {
    goto cleanup;
  }

  otter_target_execute(cstring_tests);

  if (!otter_asprintf(allocator, &string_tests_name, "%s/string_tests%s.so",
                      config->out_dir, config->suffix)) {
    goto cleanup;
  }

  string_tests = otter_target_create_c_shared_object(
      string_tests_name, exe_flags, CC_INCLUDE_FLAGS, allocator, filesystem,
      logger, (const char *[]){string_tests_src_file, NULL},
      (otter_target *[]){test_obj, string_obj, NULL});
  if (string_tests == NULL) {
    goto cleanup;
  }

  otter_target_execute(string_tests);

  if (!otter_asprintf(allocator, &array_tests_name, "%s/array_tests%s.so",
                      config->out_dir, config->suffix)) {
    goto cleanup;
  }

  array_tests = otter_target_create_c_shared_object(
      array_tests_name, exe_flags, CC_INCLUDE_FLAGS, allocator, filesystem,
      logger, (const char *[]){array_tests_src_file, NULL},
      (otter_target *[]){test_obj, array_obj, NULL});
  if (array_tests == NULL) {
    goto cleanup;
  }

  otter_target_execute(array_tests);

  if (!otter_asprintf(allocator, &lexer_tests_name, "%s/lexer_tests%s.so",
                      config->out_dir, config->suffix)) {
    goto cleanup;
  }

  lexer_tests = otter_target_create_c_shared_object(
      lexer_tests_name, exe_flags, CC_INCLUDE_FLAGS, allocator, filesystem,
      logger, (const char *[]){lexer_tests_src_file, NULL},
      (otter_target *[]){test_obj, lexer_obj, token_obj, NULL});
  if (lexer_tests == NULL) {
    goto cleanup;
  }

  otter_target_execute(lexer_tests);
  if (!otter_asprintf(allocator, &parser_tests_name, "%s/parser_tests%s.so",
                      config->out_dir, config->suffix)) {
    goto cleanup;
  }

  parser_tests = otter_target_create_c_shared_object(
      parser_tests_name, exe_flags, CC_INCLUDE_FLAGS, allocator, filesystem,
      logger, (const char *[]){parser_tests_src_file, NULL},
      (otter_target *[]){test_obj, cstring_obj, node_obj, parser_obj, NULL});
  if (parser_tests == NULL) {
    goto cleanup;
  }

  otter_target_execute(parser_tests);
  if (!otter_asprintf(allocator, &parser_int_tests_name,
                      "%s/parser_integration_tests%s.so", config->out_dir,
                      config->suffix)) {
    goto cleanup;
  }

  parser_integration_tests = otter_target_create_c_shared_object(
      parser_int_tests_name, exe_flags, CC_INCLUDE_FLAGS, allocator, filesystem,
      logger, (const char *[]){parser_integration_tests_src_file, NULL},
      (otter_target *[]){test_obj, lexer_obj, node_obj, parser_obj, NULL});
  if (parser_integration_tests == NULL) {
    goto cleanup;
  }

  otter_target_execute(parser_integration_tests);

cleanup:
  otter_free(allocator, allocator_o_file);
  otter_free(allocator, string_o_file);
  otter_free(allocator, array_o_file);
  otter_free(allocator, cstring_o_file);
  otter_free(allocator, logger_o_file);
  otter_free(allocator, file_o_file);
  otter_free(allocator, filesystem_o_file);
  otter_free(allocator, target_o_file);
  otter_free(allocator, token_o_file);
  otter_free(allocator, node_o_file);
  otter_free(allocator, lexer_o_file);
  otter_free(allocator, parser_o_file);
  otter_free(allocator, bytecode_o_file);
  otter_free(allocator, vm_o_file);
  otter_free(allocator, test_o_file);
  otter_free(allocator, allocator_src_file);
  otter_free(allocator, string_src_file);
  otter_free(allocator, array_src_file);
  otter_free(allocator, cstring_src_file);
  otter_free(allocator, logger_src_file);
  otter_free(allocator, file_src_file);
  otter_free(allocator, filesystem_src_file);
  otter_free(allocator, target_src_file);
  otter_free(allocator, token_src_file);
  otter_free(allocator, node_src_file);
  otter_free(allocator, lexer_src_file);
  otter_free(allocator, parser_src_file);
  otter_free(allocator, bytecode_src_file);
  otter_free(allocator, vm_src_file);
  otter_free(allocator, main_src_file);
  otter_free(allocator, test_src_file);
  otter_free(allocator, test_driver_src_file);
  otter_free(allocator, cstring_tests_src_file);
  otter_free(allocator, string_tests_src_file);
  otter_free(allocator, array_tests_src_file);
  otter_free(allocator, lexer_tests_src_file);
  otter_free(allocator, parser_tests_src_file);
  otter_free(allocator, parser_integration_tests_src_file);
  otter_free(allocator, exe_name);
  otter_free(allocator, exe_flags);
  otter_free(allocator, test_exe_name);
  otter_free(allocator, cstring_tests_name);
  otter_free(allocator, string_tests_name);
  otter_free(allocator, array_tests_name);
  otter_free(allocator, lexer_tests_name);
  otter_free(allocator, parser_tests_name);
  otter_free(allocator, parser_int_tests_name);
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

  char *allocator_src_file = NULL;
  char *allocator_o_file = NULL;
  char *string_src_file = NULL;
  char *string_o_file = NULL;
  char *array_src_file = NULL;
  char *array_o_file = NULL;
  char *cstring_src_file = NULL;
  char *cstring_o_file = NULL;
  char *logger_src_file = NULL;
  char *logger_o_file = NULL;
  char *file_src_file = NULL;
  char *file_o_file = NULL;
  char *filesystem_src_file = NULL;
  char *filesystem_o_file = NULL;
  char *target_src_file = NULL;
  char *target_o_file = NULL;
  char *make_src_file = NULL;
  char *make_o_file = NULL;
  char *make_flags = NULL;
  OTTER_CLEANUP(otter_target_free_p) otter_target *allocator_obj = NULL;
  OTTER_CLEANUP(otter_target_free_p) otter_target *string_obj = NULL;
  OTTER_CLEANUP(otter_target_free_p) otter_target *array_obj = NULL;
  OTTER_CLEANUP(otter_target_free_p) otter_target *cstring_obj = NULL;
  OTTER_CLEANUP(otter_target_free_p) otter_target *logger_obj = NULL;
  OTTER_CLEANUP(otter_target_free_p) otter_target *file_obj = NULL;
  OTTER_CLEANUP(otter_target_free_p) otter_target *filesystem_obj = NULL;
  OTTER_CLEANUP(otter_target_free_p) otter_target *target_obj = NULL;
  OTTER_CLEANUP(otter_target_free_p) otter_target *make_exe = NULL;

  if (!otter_asprintf(allocator, &allocator_src_file, "%s/allocator.c",
                      bootstrap_config.src_dir)) {
    return 1;
  }

  if (!otter_asprintf(allocator, &allocator_o_file, "%s/allocator.o",
                      bootstrap_config.out_dir)) {
    return 1;
  }

  if (!otter_asprintf(allocator, &string_src_file, "%s/string.c",
                      bootstrap_config.src_dir)) {
    goto bootstrap_cleanup;
  }

  if (!otter_asprintf(allocator, &string_o_file, "%s/string.o",
                      bootstrap_config.out_dir)) {
    goto bootstrap_cleanup;
  }

  if (!otter_asprintf(allocator, &array_src_file, "%s/array.c",
                      bootstrap_config.src_dir)) {
    goto bootstrap_cleanup;
  }

  if (!otter_asprintf(allocator, &array_o_file, "%s/array.o",
                      bootstrap_config.out_dir)) {
    goto bootstrap_cleanup;
  }

  if (!otter_asprintf(allocator, &cstring_src_file, "%s/cstring.c",
                      bootstrap_config.src_dir)) {
    goto bootstrap_cleanup;
  }

  if (!otter_asprintf(allocator, &cstring_o_file, "%s/cstring.o",
                      bootstrap_config.out_dir)) {
    goto bootstrap_cleanup;
  }

  if (!otter_asprintf(allocator, &logger_src_file, "%s/logger.c",
                      bootstrap_config.src_dir)) {
    goto bootstrap_cleanup;
  }

  if (!otter_asprintf(allocator, &logger_o_file, "%s/logger.o",
                      bootstrap_config.out_dir)) {
    goto bootstrap_cleanup;
  }

  if (!otter_asprintf(allocator, &file_src_file, "%s/file.c",
                      bootstrap_config.src_dir)) {
    goto bootstrap_cleanup;
  }

  if (!otter_asprintf(allocator, &file_o_file, "%s/file.o",
                      bootstrap_config.out_dir)) {
    goto bootstrap_cleanup;
  }

  if (!otter_asprintf(allocator, &filesystem_src_file, "%s/filesystem.c",
                      bootstrap_config.src_dir)) {
    goto bootstrap_cleanup;
  }

  if (!otter_asprintf(allocator, &filesystem_o_file, "%s/filesystem.o",
                      bootstrap_config.out_dir)) {
    goto bootstrap_cleanup;
  }

  if (!otter_asprintf(allocator, &target_src_file, "%s/target.c",
                      bootstrap_config.src_dir)) {
    goto bootstrap_cleanup;
  }

  if (!otter_asprintf(allocator, &target_o_file, "%s/target.o",
                      bootstrap_config.out_dir)) {
    goto bootstrap_cleanup;
  }

  if (!otter_asprintf(allocator, &make_src_file, "%s/make.c",
                      bootstrap_config.src_dir)) {
    goto bootstrap_cleanup;
  }

  if (!otter_asprintf(allocator, &make_o_file, "otter_make",
                      bootstrap_config.out_dir)) {
    goto bootstrap_cleanup;
  }

  allocator_obj = otter_target_create_c_object(
      allocator_o_file, CC_FLAGS_RELEASE, CC_INCLUDE_FLAGS, allocator,
      filesystem, logger, allocator_src_file, NULL);
  if (allocator_obj == NULL) {
    goto bootstrap_cleanup;
  }

  string_obj = otter_target_create_c_object(
      string_o_file, CC_FLAGS_RELEASE, CC_INCLUDE_FLAGS, allocator, filesystem,
      logger, string_src_file, NULL);
  if (string_obj == NULL) {
    goto bootstrap_cleanup;
  }

  otter_target_add_dependency(string_obj, allocator_obj);

  array_obj = otter_target_create_c_object(
      array_o_file, CC_FLAGS_RELEASE, CC_INCLUDE_FLAGS, allocator, filesystem,
      logger, array_src_file, NULL);
  if (array_obj == NULL) {
    goto bootstrap_cleanup;
  }

  otter_target_add_dependency(array_obj, allocator_obj);

  cstring_obj = otter_target_create_c_object(
      cstring_o_file, CC_FLAGS_RELEASE, CC_INCLUDE_FLAGS, allocator, filesystem,
      logger, cstring_src_file, NULL);
  if (cstring_obj == NULL) {
    goto bootstrap_cleanup;
  }

  otter_target_add_dependency(cstring_obj, allocator_obj);

  logger_obj = otter_target_create_c_object(
      logger_o_file, CC_FLAGS_RELEASE, CC_INCLUDE_FLAGS, allocator, filesystem,
      logger, logger_src_file, NULL);
  if (logger_obj == NULL) {
    goto bootstrap_cleanup;
  }

  otter_target_add_dependency(logger_obj, cstring_obj);
  otter_target_add_dependency(logger_obj, array_obj);
  otter_target_add_dependency(logger_obj, allocator_obj);

  file_obj = otter_target_create_c_object(
      file_o_file, CC_FLAGS_RELEASE, CC_INCLUDE_FLAGS, allocator, filesystem,
      logger, file_src_file, NULL);
  if (file_obj == NULL) {
    goto bootstrap_cleanup;
  }

  filesystem_obj = otter_target_create_c_object(
      filesystem_o_file, CC_FLAGS_RELEASE, CC_INCLUDE_FLAGS, allocator,
      filesystem, logger, filesystem_src_file, NULL);
  if (filesystem_obj == NULL) {
    goto bootstrap_cleanup;
  }

  otter_target_add_dependency(filesystem_obj, file_obj);
  otter_target_add_dependency(filesystem_obj, allocator_obj);

  target_obj = otter_target_create_c_object(
      target_o_file, CC_FLAGS_RELEASE, CC_INCLUDE_FLAGS, allocator, filesystem,
      logger, target_src_file, NULL);
  if (target_obj == NULL) {
    goto bootstrap_cleanup;
  }
  otter_target_add_dependency(target_obj, allocator_obj);
  otter_target_add_dependency(target_obj, array_obj);
  otter_target_add_dependency(target_obj, filesystem_obj);
  otter_target_add_dependency(target_obj, logger_obj);
  otter_target_add_dependency(target_obj, string_obj);

  if (!otter_asprintf(allocator, &make_flags, "%s -lgnutls",
                      CC_FLAGS_RELEASE)) {
    goto bootstrap_cleanup;
  }

  make_exe = otter_target_create_c_executable(
      make_o_file, make_flags, CC_INCLUDE_FLAGS, allocator, filesystem, logger,
      (const char *[]){make_src_file, NULL},
      (otter_target *[]){allocator_obj, filesystem_obj, logger_obj, target_obj,
                         NULL});
  if (make_exe == NULL) {
    goto bootstrap_cleanup;
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

bootstrap_cleanup:
  otter_free(allocator, allocator_src_file);
  otter_free(allocator, allocator_o_file);
  otter_free(allocator, string_src_file);
  otter_free(allocator, string_o_file);
  otter_free(allocator, array_src_file);
  otter_free(allocator, array_o_file);
  otter_free(allocator, cstring_src_file);
  otter_free(allocator, cstring_o_file);
  otter_free(allocator, logger_src_file);
  otter_free(allocator, logger_o_file);
  otter_free(allocator, file_src_file);
  otter_free(allocator, file_o_file);
  otter_free(allocator, filesystem_src_file);
  otter_free(allocator, filesystem_o_file);
  otter_free(allocator, target_src_file);
  otter_free(allocator, target_o_file);
  otter_free(allocator, make_src_file);
  otter_free(allocator, make_o_file);
  otter_free(allocator, make_flags);
  return 0;
}
