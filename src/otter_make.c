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
#include "otter_cstring.h"
#include "otter_filesystem.h"
#include "otter_inc.h"
#include "otter_logger.h"
#include "otter_target.h"

#include <stddef.h>
#include <stdio.h>
#include <string.h>

#define CC_INCLUDE_FLAGS "-I ./include/otter "
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
  char *otter_allocator_o_file = NULL;
  char *otter_string_o_file = NULL;
  char *otter_array_o_file = NULL;
  char *otter_cstring_o_file = NULL;
  char *otter_logger_o_file = NULL;
  char *otter_file_o_file = NULL;
  char *otter_filesystem_o_file = NULL;
  char *otter_target_o_file = NULL;
  char *otter_token_o_file = NULL;
  char *otter_node_o_file = NULL;
  char *otter_lexer_o_file = NULL;
  char *otter_parser_o_file = NULL;
  char *otter_bytecode_o_file = NULL;
  char *otter_vm_o_file = NULL;
  char *otter_test_o_file = NULL;
  char *otter_allocator_src_file = NULL;
  char *otter_string_src_file = NULL;
  char *otter_array_src_file = NULL;
  char *otter_cstring_src_file = NULL;
  char *otter_logger_src_file = NULL;
  char *otter_file_src_file = NULL;
  char *otter_filesystem_src_file = NULL;
  char *otter_target_src_file = NULL;
  char *otter_token_src_file = NULL;
  char *otter_node_src_file = NULL;
  char *otter_lexer_src_file = NULL;
  char *otter_parser_src_file = NULL;
  char *otter_bytecode_src_file = NULL;
  char *otter_vm_src_file = NULL;
  char *otter_main_src_file = NULL;
  char *otter_test_src_file = NULL;
  char *otter_test_driver_src_file = NULL;
  char *otter_cstring_tests_src_file = NULL;
  char *otter_array_tests_src_file = NULL;
  char *otter_lexer_tests_src_file = NULL;
  char *otter_parser_tests_src_file = NULL;
  char *otter_parser_integration_tests_src_file = NULL;
  char *exe_name = NULL;
  char *exe_flags = NULL;
  char *test_exe_name = NULL;
  char *cstring_tests_name = NULL;
  char *array_tests_name = NULL;
  char *lexer_tests_name = NULL;
  char *parser_tests_name = NULL;
  char *parser_int_tests_name = NULL;
  OTTER_CLEANUP(otter_target_free_p) otter_target *otter_allocator_obj = NULL;
  OTTER_CLEANUP(otter_target_free_p) otter_target *otter_string_obj = NULL;
  OTTER_CLEANUP(otter_target_free_p) otter_target *otter_array_obj = NULL;
  OTTER_CLEANUP(otter_target_free_p) otter_target *otter_cstring_obj = NULL;
  OTTER_CLEANUP(otter_target_free_p) otter_target *otter_logger_obj = NULL;
  OTTER_CLEANUP(otter_target_free_p) otter_target *otter_file_obj = NULL;
  OTTER_CLEANUP(otter_target_free_p) otter_target *otter_filesystem_obj = NULL;
  OTTER_CLEANUP(otter_target_free_p) otter_target *otter_target_obj = NULL;
  OTTER_CLEANUP(otter_target_free_p) otter_target *otter_token_obj = NULL;
  OTTER_CLEANUP(otter_target_free_p) otter_target *otter_node_obj = NULL;
  OTTER_CLEANUP(otter_target_free_p)
  otter_target *otter_otter_lexer_o_filebj = NULL;
  OTTER_CLEANUP(otter_target_free_p) otter_target *otter_parser_obj = NULL;
  OTTER_CLEANUP(otter_target_free_p) otter_target *otter_bytecode_obj = NULL;
  OTTER_CLEANUP(otter_target_free_p) otter_target *otter_vm_obj = NULL;
  OTTER_CLEANUP(otter_target_free_p) otter_target *otter_exe = NULL;
  OTTER_CLEANUP(otter_target_free_p) otter_target *otter_test_obj = NULL;
  OTTER_CLEANUP(otter_target_free_p) otter_target *otter_test_driver = NULL;
  OTTER_CLEANUP(otter_target_free_p) otter_target *otter_cstring_tests = NULL;
  OTTER_CLEANUP(otter_target_free_p) otter_target *otter_array_tests = NULL;
  OTTER_CLEANUP(otter_target_free_p) otter_target *otter_lexer_tests = NULL;
  OTTER_CLEANUP(otter_target_free_p) otter_target *otter_parser_tests = NULL;
  OTTER_CLEANUP(otter_target_free_p)
  otter_target *otter_parser_integration_tests = NULL;

  if (!otter_asprintf(allocator, &otter_allocator_o_file,
                      "%s/otter_allocator%s.o", config->out_dir,
                      config->suffix)) {
    goto cleanup;
  }

  if (!otter_asprintf(allocator, &otter_string_o_file, "%s/otter_string%s.o",
                      config->out_dir, config->suffix)) {
    goto cleanup;
  }

  if (!otter_asprintf(allocator, &otter_array_o_file, "%s/otter_array%s.o",
                      config->out_dir, config->suffix)) {
    goto cleanup;
  }

  if (!otter_asprintf(allocator, &otter_cstring_o_file, "%s/otter_cstring%s.o",
                      config->out_dir, config->suffix)) {
    goto cleanup;
  }

  if (!otter_asprintf(allocator, &otter_logger_o_file, "%s/otter_logger%s.o",
                      config->out_dir, config->suffix)) {
    goto cleanup;
  }

  if (!otter_asprintf(allocator, &otter_file_o_file, "%s/otter_file%s.o",
                      config->out_dir, config->suffix)) {
    goto cleanup;
  }

  if (!otter_asprintf(allocator, &otter_filesystem_o_file,
                      "%s/otter_filesystem%s.o", config->out_dir,
                      config->suffix)) {
    goto cleanup;
  }

  if (!otter_asprintf(allocator, &otter_target_o_file, "%s/otter_target%s.o",
                      config->out_dir, config->suffix)) {
    goto cleanup;
  }

  if (!otter_asprintf(allocator, &otter_token_o_file, "%s/otter_token%s.o",
                      config->out_dir, config->suffix)) {
    goto cleanup;
  }

  if (!otter_asprintf(allocator, &otter_node_o_file, "%s/otter_node%s.o",
                      config->out_dir, config->suffix)) {
    goto cleanup;
  }

  if (!otter_asprintf(allocator, &otter_lexer_o_file, "%s/otter_lexer%s.o",
                      config->out_dir, config->suffix)) {
    goto cleanup;
  }

  if (!otter_asprintf(allocator, &otter_parser_o_file, "%s/otter_parser%s.o",
                      config->out_dir, config->suffix)) {
    goto cleanup;
  }

  if (!otter_asprintf(allocator, &otter_bytecode_o_file,
                      "%s/otter_bytecode%s.o", config->out_dir,
                      config->suffix)) {
    goto cleanup;
  }

  if (!otter_asprintf(allocator, &otter_vm_o_file, "%s/otter_vm%s.o",
                      config->out_dir, config->suffix)) {
    goto cleanup;
  }

  if (!otter_asprintf(allocator, &otter_test_o_file, "%s/otter_test%s.o",
                      config->out_dir, config->suffix)) {
    goto cleanup;
  }

  if (!otter_asprintf(allocator, &otter_allocator_src_file,
                      "%s/otter_allocator.c", config->src_dir)) {
    goto cleanup;
  }

  if (!otter_asprintf(allocator, &otter_string_src_file, "%s/otter_string.c",
                      config->src_dir)) {
    goto cleanup;
  }

  if (!otter_asprintf(allocator, &otter_array_src_file, "%s/otter_array.c",
                      config->src_dir)) {
    goto cleanup;
  }

  if (!otter_asprintf(allocator, &otter_cstring_src_file, "%s/otter_cstring.c",
                      config->src_dir)) {
    goto cleanup;
  }

  if (!otter_asprintf(allocator, &otter_logger_src_file, "%s/otter_logger.c",
                      config->src_dir)) {
    goto cleanup;
  }

  if (!otter_asprintf(allocator, &otter_file_src_file, "%s/otter_file.c",
                      config->src_dir)) {
    goto cleanup;
  }

  if (!otter_asprintf(allocator, &otter_filesystem_src_file,
                      "%s/otter_filesystem.c", config->src_dir)) {
    goto cleanup;
  }

  if (!otter_asprintf(allocator, &otter_target_src_file, "%s/otter_target.c",
                      config->src_dir)) {
    goto cleanup;
  }

  if (!otter_asprintf(allocator, &otter_token_src_file, "%s/otter_token.c",
                      config->src_dir)) {
    goto cleanup;
  }

  if (!otter_asprintf(allocator, &otter_node_src_file, "%s/otter_node.c",
                      config->src_dir)) {
    goto cleanup;
  }

  if (!otter_asprintf(allocator, &otter_lexer_src_file, "%s/otter_lexer.c",
                      config->src_dir)) {
    goto cleanup;
  }

  if (!otter_asprintf(allocator, &otter_parser_src_file, "%s/otter_parser.c",
                      config->src_dir)) {
    goto cleanup;
  }

  if (!otter_asprintf(allocator, &otter_bytecode_src_file,
                      "%s/otter_bytecode.c", config->src_dir)) {
    goto cleanup;
  }

  if (!otter_asprintf(allocator, &otter_vm_src_file, "%s/otter_vm.c",
                      config->src_dir)) {
    goto cleanup;
  }

  if (!otter_asprintf(allocator, &otter_main_src_file, "%s/otter.c",
                      config->src_dir)) {
    goto cleanup;
  }

  if (!otter_asprintf(allocator, &otter_test_src_file, "%s/otter_test.c",
                      config->src_dir)) {
    goto cleanup;
  }

  if (!otter_asprintf(allocator, &otter_test_driver_src_file,
                      "%s/otter_test_driver.c", config->src_dir)) {
    goto cleanup;
  }

  if (!otter_asprintf(allocator, &otter_cstring_tests_src_file,
                      "%s/otter_cstring_tests.c", config->src_dir)) {
    goto cleanup;
  }

  if (!otter_asprintf(allocator, &otter_array_tests_src_file,
                      "%s/otter_array_tests.c", config->src_dir)) {
    goto cleanup;
  }

  if (!otter_asprintf(allocator, &otter_lexer_tests_src_file,
                      "%s/otter_lexer_tests.c", config->src_dir)) {
    goto cleanup;
  }

  if (!otter_asprintf(allocator, &otter_parser_tests_src_file,
                      "%s/otter_parser_tests.c", config->src_dir)) {
    goto cleanup;
  }

  if (!otter_asprintf(allocator, &otter_parser_integration_tests_src_file,
                      "%s/otter_parser_integration_tests.c", config->src_dir)) {
    goto cleanup;
  }

  otter_allocator_obj = otter_target_create_c_object(
      otter_allocator_o_file, cc_flags, CC_INCLUDE_FLAGS, allocator, filesystem,
      logger, otter_allocator_src_file, NULL);
  if (otter_allocator_obj == NULL) {
    goto cleanup;
  }

  otter_string_obj = otter_target_create_c_object(
      otter_string_o_file, cc_flags, CC_INCLUDE_FLAGS, allocator, filesystem,
      logger, otter_string_src_file, NULL);
  if (otter_string_obj == NULL) {
    goto cleanup;
  }

  otter_target_add_dependency(otter_string_obj, otter_allocator_obj);

  otter_array_obj = otter_target_create_c_object(
      otter_array_o_file, cc_flags, CC_INCLUDE_FLAGS, allocator, filesystem,
      logger, otter_array_src_file, NULL);
  if (otter_array_obj == NULL) {
    goto cleanup;
  }

  otter_target_add_dependency(otter_array_obj, otter_allocator_obj);

  otter_cstring_obj = otter_target_create_c_object(
      otter_cstring_o_file, cc_flags, CC_INCLUDE_FLAGS, allocator, filesystem,
      logger, otter_cstring_src_file, NULL);
  if (otter_cstring_obj == NULL) {
    goto cleanup;
  }

  otter_target_add_dependency(otter_cstring_obj, otter_allocator_obj);

  otter_logger_obj = otter_target_create_c_object(
      otter_logger_o_file, cc_flags, CC_INCLUDE_FLAGS, allocator, filesystem,
      logger, otter_logger_src_file, NULL);
  if (otter_logger_obj == NULL) {
    goto cleanup;
  }

  otter_target_add_dependency(otter_logger_obj, otter_cstring_obj);
  otter_target_add_dependency(otter_logger_obj, otter_array_obj);
  otter_target_add_dependency(otter_logger_obj, otter_allocator_obj);

  otter_file_obj = otter_target_create_c_object(
      otter_file_o_file, cc_flags, CC_INCLUDE_FLAGS, allocator, filesystem,
      logger, otter_file_src_file, NULL);
  if (otter_file_obj == NULL) {
    goto cleanup;
  }

  otter_filesystem_obj = otter_target_create_c_object(
      otter_filesystem_o_file, cc_flags, CC_INCLUDE_FLAGS, allocator,
      filesystem, logger, otter_filesystem_src_file, NULL);
  if (otter_filesystem_obj == NULL) {
    goto cleanup;
  }

  otter_target_add_dependency(otter_filesystem_obj, otter_file_obj);
  otter_target_add_dependency(otter_filesystem_obj, otter_allocator_obj);

  otter_target_obj = otter_target_create_c_object(
      otter_target_o_file, cc_flags, CC_INCLUDE_FLAGS, allocator, filesystem,
      logger, otter_target_src_file, NULL);
  if (otter_target_obj == NULL) {
    goto cleanup;
  }

  otter_target_add_dependency(otter_target_obj, otter_allocator_obj);
  otter_target_add_dependency(otter_target_obj, otter_array_obj);
  otter_target_add_dependency(otter_target_obj, otter_filesystem_obj);
  otter_target_add_dependency(otter_target_obj, otter_logger_obj);
  otter_target_add_dependency(otter_target_obj, otter_string_obj);

  otter_token_obj = otter_target_create_c_object(
      otter_token_o_file, cc_flags, CC_INCLUDE_FLAGS, allocator, filesystem,
      logger, otter_token_src_file, NULL);
  if (otter_token_obj == NULL) {
    goto cleanup;
  }

  otter_target_add_dependency(otter_token_obj, otter_allocator_obj);

  otter_node_obj = otter_target_create_c_object(
      otter_node_o_file, cc_flags, CC_INCLUDE_FLAGS, allocator, filesystem,
      logger, otter_node_src_file, NULL);
  if (otter_node_obj == NULL) {
    goto cleanup;
  }

  otter_target_add_dependency(otter_node_obj, otter_allocator_obj);
  otter_target_add_dependency(otter_node_obj, otter_array_obj);

  otter_otter_lexer_o_filebj = otter_target_create_c_object(
      otter_lexer_o_file, cc_flags, CC_INCLUDE_FLAGS, allocator, filesystem,
      logger, otter_lexer_src_file, NULL);
  if (otter_otter_lexer_o_filebj == NULL) {
    goto cleanup;
  }

  otter_target_add_dependency(otter_otter_lexer_o_filebj, otter_array_obj);
  otter_target_add_dependency(otter_otter_lexer_o_filebj, otter_cstring_obj);

  otter_parser_obj = otter_target_create_c_object(
      otter_parser_o_file, cc_flags, CC_INCLUDE_FLAGS, allocator, filesystem,
      logger, otter_parser_src_file, NULL);
  if (otter_parser_obj == NULL) {
    goto cleanup;
  }

  otter_target_add_dependency(otter_parser_obj, otter_allocator_obj);
  otter_target_add_dependency(otter_parser_obj, otter_logger_obj);
  otter_target_add_dependency(otter_parser_obj, otter_node_obj);
  otter_target_add_dependency(otter_parser_obj, otter_cstring_obj);
  otter_target_add_dependency(otter_parser_obj, otter_token_obj);

  otter_bytecode_obj = otter_target_create_c_object(
      otter_bytecode_o_file, cc_flags, CC_INCLUDE_FLAGS, allocator, filesystem,
      logger, otter_bytecode_src_file, NULL);
  if (otter_bytecode_obj == NULL) {
    goto cleanup;
  }

  otter_vm_obj = otter_target_create_c_object(
      otter_vm_o_file, cc_flags, CC_INCLUDE_FLAGS, allocator, filesystem,
      logger, otter_vm_src_file, NULL);
  if (otter_vm_obj == NULL) {
    goto cleanup;
  }

  otter_target_add_dependency(otter_vm_obj, otter_allocator_obj);
  otter_target_add_dependency(otter_vm_obj, otter_logger_obj);
  otter_target_add_dependency(otter_vm_obj, otter_bytecode_obj);

  /* Main executable */
  if (!otter_asprintf(allocator, &exe_name, "%s/otter%s", config->out_dir,
                      config->suffix)) {
    goto cleanup;
  }

  if (!otter_asprintf(allocator, &exe_flags, "%s%s", cc_flags, ll_flags)) {
    goto cleanup;
  }

  otter_exe = otter_target_create_c_executable(
      exe_name, exe_flags, CC_INCLUDE_FLAGS, allocator, filesystem, logger,
      (const char *[]){otter_main_src_file, NULL},
      (otter_target *[]){otter_vm_obj, NULL});
  if (otter_exe == NULL) {
    goto cleanup;
  }

  otter_target_execute(otter_exe);

  /* Test driver */
  otter_test_obj = otter_target_create_c_object(
      otter_test_o_file, cc_flags, CC_INCLUDE_FLAGS, allocator, filesystem,
      logger, otter_test_src_file, NULL);
  if (otter_test_obj == NULL) {
    goto cleanup;
  }

  otter_target_add_dependency(otter_test_obj, otter_allocator_obj);

  if (!otter_asprintf(allocator, &test_exe_name, "%s/otter_test%s",
                      config->out_dir, config->suffix)) {
    goto cleanup;
  }

  otter_test_driver = otter_target_create_c_executable(
      test_exe_name, exe_flags, CC_INCLUDE_FLAGS, allocator, filesystem, logger,
      (const char *[]){otter_test_driver_src_file, NULL},
      (otter_target *[]){otter_allocator_obj, NULL});
  if (otter_test_driver == NULL) {
    goto cleanup;
  }

  otter_target_execute(otter_test_driver);

  /* Test shared objects */
  if (!otter_asprintf(allocator, &cstring_tests_name,
                      "%s/otter_cstring_tests%s.so", config->out_dir,
                      config->suffix)) {
    goto cleanup;
  }

  otter_cstring_tests = otter_target_create_c_shared_object(
      cstring_tests_name, cc_flags, CC_INCLUDE_FLAGS, allocator, filesystem,
      logger, (const char *[]){otter_cstring_tests_src_file, NULL},
      (otter_target *[]){otter_test_obj, otter_cstring_obj, NULL});
  if (otter_cstring_tests == NULL) {
    goto cleanup;
  }

  otter_target_execute(otter_cstring_tests);
  if (!otter_asprintf(allocator, &array_tests_name, "%s/otter_array_tests%s.so",
                      config->out_dir, config->suffix)) {
    goto cleanup;
  }

  otter_array_tests = otter_target_create_c_shared_object(
      array_tests_name, exe_flags, CC_INCLUDE_FLAGS, allocator, filesystem,
      logger, (const char *[]){otter_array_tests_src_file, NULL},
      (otter_target *[]){otter_test_obj, otter_array_obj, NULL});
  if (otter_array_tests == NULL) {
    goto cleanup;
  }

  otter_target_execute(otter_array_tests);

  if (!otter_asprintf(allocator, &lexer_tests_name, "%s/otter_lexer_tests%s.so",
                      config->out_dir, config->suffix)) {
    goto cleanup;
  }

  otter_lexer_tests = otter_target_create_c_shared_object(
      lexer_tests_name, exe_flags, CC_INCLUDE_FLAGS, allocator, filesystem,
      logger, (const char *[]){otter_lexer_tests_src_file, NULL},
      (otter_target *[]){otter_test_obj, otter_otter_lexer_o_filebj,
                         otter_token_obj, NULL});
  if (otter_lexer_tests == NULL) {
    goto cleanup;
  }

  otter_target_execute(otter_lexer_tests);
  if (!otter_asprintf(allocator, &parser_tests_name,
                      "%s/otter_parser_tests%s.so", config->out_dir,
                      config->suffix)) {
    goto cleanup;
  }

  otter_parser_tests = otter_target_create_c_shared_object(
      parser_tests_name, exe_flags, CC_INCLUDE_FLAGS, allocator, filesystem,
      logger, (const char *[]){otter_parser_tests_src_file, NULL},
      (otter_target *[]){otter_test_obj, otter_cstring_obj, otter_node_obj,
                         otter_parser_obj, NULL});
  if (otter_parser_tests == NULL) {
    goto cleanup;
  }

  otter_target_execute(otter_parser_tests);
  if (!otter_asprintf(allocator, &parser_int_tests_name,
                      "%s/otter_parser_integration_tests%s.so", config->out_dir,
                      config->suffix)) {
    goto cleanup;
  }

  otter_parser_integration_tests = otter_target_create_c_shared_object(
      parser_int_tests_name, exe_flags, CC_INCLUDE_FLAGS, allocator, filesystem,
      logger, (const char *[]){otter_parser_integration_tests_src_file, NULL},
      (otter_target *[]){otter_test_obj, otter_otter_lexer_o_filebj,
                         otter_node_obj, otter_parser_obj, NULL});
  if (otter_parser_integration_tests == NULL) {
    goto cleanup;
  }

  otter_target_execute(otter_parser_integration_tests);

cleanup:
  otter_free(allocator, otter_allocator_o_file);
  otter_free(allocator, otter_string_o_file);
  otter_free(allocator, otter_array_o_file);
  otter_free(allocator, otter_cstring_o_file);
  otter_free(allocator, otter_logger_o_file);
  otter_free(allocator, otter_file_o_file);
  otter_free(allocator, otter_filesystem_o_file);
  otter_free(allocator, otter_target_o_file);
  otter_free(allocator, otter_token_o_file);
  otter_free(allocator, otter_node_o_file);
  otter_free(allocator, otter_lexer_o_file);
  otter_free(allocator, otter_parser_o_file);
  otter_free(allocator, otter_bytecode_o_file);
  otter_free(allocator, otter_vm_o_file);
  otter_free(allocator, otter_test_o_file);
  otter_free(allocator, otter_allocator_src_file);
  otter_free(allocator, otter_string_src_file);
  otter_free(allocator, otter_array_src_file);
  otter_free(allocator, otter_cstring_src_file);
  otter_free(allocator, otter_logger_src_file);
  otter_free(allocator, otter_file_src_file);
  otter_free(allocator, otter_filesystem_src_file);
  otter_free(allocator, otter_target_src_file);
  otter_free(allocator, otter_token_src_file);
  otter_free(allocator, otter_node_src_file);
  otter_free(allocator, otter_lexer_src_file);
  otter_free(allocator, otter_parser_src_file);
  otter_free(allocator, otter_bytecode_src_file);
  otter_free(allocator, otter_vm_src_file);
  otter_free(allocator, otter_main_src_file);
  otter_free(allocator, otter_test_src_file);
  otter_free(allocator, otter_test_driver_src_file);
  otter_free(allocator, otter_cstring_tests_src_file);
  otter_free(allocator, otter_array_tests_src_file);
  otter_free(allocator, otter_lexer_tests_src_file);
  otter_free(allocator, otter_parser_tests_src_file);
  otter_free(allocator, otter_parser_integration_tests_src_file);
  otter_free(allocator, exe_name);
  otter_free(allocator, exe_flags);
  otter_free(allocator, test_exe_name);
  otter_free(allocator, cstring_tests_name);
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

  char *otter_allocator_src_file = NULL;
  char *otter_allocator_o_file = NULL;
  char *otter_string_src_file = NULL;
  char *otter_string_o_file = NULL;
  char *otter_array_src_file = NULL;
  char *otter_array_o_file = NULL;
  char *otter_cstring_src_file = NULL;
  char *otter_cstring_o_file = NULL;
  char *otter_logger_src_file = NULL;
  char *otter_logger_o_file = NULL;
  char *otter_file_src_file = NULL;
  char *otter_file_o_file = NULL;
  char *otter_filesystem_src_file = NULL;
  char *otter_filesystem_o_file = NULL;
  char *otter_target_src_file = NULL;
  char *otter_target_o_file = NULL;
  char *otter_make_src_file = NULL;
  char *otter_make_o_file = NULL;
  char *make_flags = NULL;
  OTTER_CLEANUP(otter_target_free_p) otter_target *otter_allocator_obj = NULL;
  OTTER_CLEANUP(otter_target_free_p) otter_target *otter_string_obj = NULL;
  OTTER_CLEANUP(otter_target_free_p) otter_target *otter_array_obj = NULL;
  OTTER_CLEANUP(otter_target_free_p) otter_target *otter_cstring_obj = NULL;
  OTTER_CLEANUP(otter_target_free_p) otter_target *otter_logger_obj = NULL;
  OTTER_CLEANUP(otter_target_free_p) otter_target *otter_file_obj = NULL;
  OTTER_CLEANUP(otter_target_free_p) otter_target *otter_filesystem_obj = NULL;
  OTTER_CLEANUP(otter_target_free_p) otter_target *otter_target_obj = NULL;
  OTTER_CLEANUP(otter_target_free_p) otter_target *otter_make_exe = NULL;

  if (!otter_asprintf(allocator, &otter_allocator_src_file,
                      "%s/otter_allocator.c", bootstrap_config.src_dir)) {
    return 1;
  }

  if (!otter_asprintf(allocator, &otter_allocator_o_file,
                      "%s/otter_allocator.o", bootstrap_config.out_dir)) {
    return 1;
  }

  if (!otter_asprintf(allocator, &otter_string_src_file, "%s/otter_string.c",
                      bootstrap_config.src_dir)) {
    goto bootstrap_cleanup;
  }

  if (!otter_asprintf(allocator, &otter_string_o_file, "%s/otter_string.o",
                      bootstrap_config.out_dir)) {
    goto bootstrap_cleanup;
  }

  if (!otter_asprintf(allocator, &otter_array_src_file, "%s/otter_array.c",
                      bootstrap_config.src_dir)) {
    goto bootstrap_cleanup;
  }

  if (!otter_asprintf(allocator, &otter_array_o_file, "%s/otter_array.o",
                      bootstrap_config.out_dir)) {
    goto bootstrap_cleanup;
  }

  if (!otter_asprintf(allocator, &otter_cstring_src_file, "%s/otter_cstring.c",
                      bootstrap_config.src_dir)) {
    goto bootstrap_cleanup;
  }

  if (!otter_asprintf(allocator, &otter_cstring_o_file, "%s/otter_cstring.o",
                      bootstrap_config.out_dir)) {
    goto bootstrap_cleanup;
  }

  if (!otter_asprintf(allocator, &otter_logger_src_file, "%s/otter_logger.c",
                      bootstrap_config.src_dir)) {
    goto bootstrap_cleanup;
  }

  if (!otter_asprintf(allocator, &otter_logger_o_file, "%s/otter_logger.o",
                      bootstrap_config.out_dir)) {
    goto bootstrap_cleanup;
  }

  if (!otter_asprintf(allocator, &otter_file_src_file, "%s/otter_file.c",
                      bootstrap_config.src_dir)) {
    goto bootstrap_cleanup;
  }

  if (!otter_asprintf(allocator, &otter_file_o_file, "%s/otter_file.o",
                      bootstrap_config.out_dir)) {
    goto bootstrap_cleanup;
  }

  if (!otter_asprintf(allocator, &otter_filesystem_src_file,
                      "%s/otter_filesystem.c", bootstrap_config.src_dir)) {
    goto bootstrap_cleanup;
  }

  if (!otter_asprintf(allocator, &otter_filesystem_o_file,
                      "%s/otter_filesystem.o", bootstrap_config.out_dir)) {
    goto bootstrap_cleanup;
  }

  if (!otter_asprintf(allocator, &otter_target_src_file, "%s/otter_target.c",
                      bootstrap_config.src_dir)) {
    goto bootstrap_cleanup;
  }

  if (!otter_asprintf(allocator, &otter_target_o_file, "%s/otter_target.o",
                      bootstrap_config.out_dir)) {
    goto bootstrap_cleanup;
  }

  if (!otter_asprintf(allocator, &otter_make_src_file, "%s/otter_make.c",
                      bootstrap_config.src_dir)) {
    goto bootstrap_cleanup;
  }

  if (!otter_asprintf(allocator, &otter_make_o_file, "otter_make",
                      bootstrap_config.out_dir)) {
    goto bootstrap_cleanup;
  }

  otter_allocator_obj = otter_target_create_c_object(
      otter_allocator_o_file, CC_FLAGS_RELEASE, CC_INCLUDE_FLAGS, allocator,
      filesystem, logger, otter_allocator_src_file, NULL);
  if (otter_allocator_obj == NULL) {
    goto bootstrap_cleanup;
  }

  otter_string_obj = otter_target_create_c_object(
      otter_string_o_file, CC_FLAGS_RELEASE, CC_INCLUDE_FLAGS, allocator,
      filesystem, logger, otter_string_src_file, NULL);
  if (otter_string_obj == NULL) {
    goto bootstrap_cleanup;
  }

  otter_target_add_dependency(otter_string_obj, otter_allocator_obj);

  otter_array_obj = otter_target_create_c_object(
      otter_array_o_file, CC_FLAGS_RELEASE, CC_INCLUDE_FLAGS, allocator,
      filesystem, logger, otter_array_src_file, NULL);
  if (otter_array_obj == NULL) {
    goto bootstrap_cleanup;
  }

  otter_target_add_dependency(otter_array_obj, otter_allocator_obj);

  otter_cstring_obj = otter_target_create_c_object(
      otter_cstring_o_file, CC_FLAGS_RELEASE, CC_INCLUDE_FLAGS, allocator,
      filesystem, logger, otter_cstring_src_file, NULL);
  if (otter_cstring_obj == NULL) {
    goto bootstrap_cleanup;
  }

  otter_target_add_dependency(otter_cstring_obj, otter_allocator_obj);

  otter_logger_obj = otter_target_create_c_object(
      otter_logger_o_file, CC_FLAGS_RELEASE, CC_INCLUDE_FLAGS, allocator,
      filesystem, logger, otter_logger_src_file, NULL);
  if (otter_logger_obj == NULL) {
    goto bootstrap_cleanup;
  }

  otter_target_add_dependency(otter_logger_obj, otter_cstring_obj);
  otter_target_add_dependency(otter_logger_obj, otter_array_obj);
  otter_target_add_dependency(otter_logger_obj, otter_allocator_obj);

  otter_file_obj = otter_target_create_c_object(
      otter_file_o_file, CC_FLAGS_RELEASE, CC_INCLUDE_FLAGS, allocator,
      filesystem, logger, otter_file_src_file, NULL);
  if (otter_file_obj == NULL) {
    goto bootstrap_cleanup;
  }

  otter_filesystem_obj = otter_target_create_c_object(
      otter_filesystem_o_file, CC_FLAGS_RELEASE, CC_INCLUDE_FLAGS, allocator,
      filesystem, logger, otter_filesystem_src_file, NULL);
  if (otter_filesystem_obj == NULL) {
    goto bootstrap_cleanup;
  }

  otter_target_add_dependency(otter_filesystem_obj, otter_file_obj);
  otter_target_add_dependency(otter_filesystem_obj, otter_allocator_obj);

  otter_target_obj = otter_target_create_c_object(
      otter_target_o_file, CC_FLAGS_RELEASE, CC_INCLUDE_FLAGS, allocator,
      filesystem, logger, otter_target_src_file, NULL);
  if (otter_target_obj == NULL) {
    goto bootstrap_cleanup;
  }
  otter_target_add_dependency(otter_target_obj, otter_allocator_obj);
  otter_target_add_dependency(otter_target_obj, otter_array_obj);
  otter_target_add_dependency(otter_target_obj, otter_filesystem_obj);
  otter_target_add_dependency(otter_target_obj, otter_logger_obj);
  otter_target_add_dependency(otter_target_obj, otter_string_obj);

  if (!otter_asprintf(allocator, &make_flags, "%s -lgnutls",
                      CC_FLAGS_RELEASE)) {
    goto bootstrap_cleanup;
  }

  otter_make_exe = otter_target_create_c_executable(
      otter_make_o_file, make_flags, CC_INCLUDE_FLAGS, allocator, filesystem,
      logger, (const char *[]){otter_make_src_file, NULL},
      (otter_target *[]){otter_allocator_obj, otter_filesystem_obj,
                         otter_logger_obj, otter_target_obj, NULL});
  if (otter_make_exe == NULL) {
    goto bootstrap_cleanup;
  }

  otter_target_execute(otter_make_exe);

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
  otter_free(allocator, otter_allocator_src_file);
  otter_free(allocator, otter_allocator_o_file);
  otter_free(allocator, otter_string_src_file);
  otter_free(allocator, otter_string_o_file);
  otter_free(allocator, otter_array_src_file);
  otter_free(allocator, otter_array_o_file);
  otter_free(allocator, otter_cstring_src_file);
  otter_free(allocator, otter_cstring_o_file);
  otter_free(allocator, otter_logger_src_file);
  otter_free(allocator, otter_logger_o_file);
  otter_free(allocator, otter_file_src_file);
  otter_free(allocator, otter_file_o_file);
  otter_free(allocator, otter_filesystem_src_file);
  otter_free(allocator, otter_filesystem_o_file);
  otter_free(allocator, otter_target_src_file);
  otter_free(allocator, otter_target_o_file);
  otter_free(allocator, otter_make_src_file);
  otter_free(allocator, otter_make_o_file);
  otter_free(allocator, make_flags);
  return 0;
}
