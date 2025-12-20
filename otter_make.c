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

#define CC_FLAGS_COMMON "-std=c23 -Wall -Wextra -Werror -Wshadow -Wconversion "

#define CC_FLAGS_DEBUG CC_FLAGS_COMMON "-g -fsanitize=address,undefined "
#define CC_FLAGS_COVERAGE CC_FLAGS_COMMON "-fprofile-arcs -ftest-coverage"

#define CC_FLAGS CC_FLAGS_DEBUG

int main() {
  OTTER_CLEANUP(otter_allocator_free_p)
  otter_allocator *allocator = otter_allocator_create();

  OTTER_CLEANUP(otter_logger_free_p)
  otter_logger *logger = otter_logger_create(allocator, OTTER_LOG_LEVEL_INFO);
  otter_logger_add_sink(logger, otter_logger_console_sink);

  OTTER_CLEANUP(otter_filesystem_free_p)
  otter_filesystem *filesystem = otter_filesystem_create(allocator);

  /* Build the build system */
  OTTER_CLEANUP(otter_target_free_p)
  otter_target *otter_allocator_obj = otter_target_create(
      "otter_allocator.o", allocator, filesystem, logger, "otter_allocator.c",
      "otter_allocator.h", "otter_inc.h", NULL);
  otter_target_add_command(
      otter_allocator_obj,
      "cc -fPIC -c otter_allocator.c -o otter_allocator.o " CC_FLAGS);

  OTTER_CLEANUP(otter_target_free_p)
  otter_target *otter_logger_obj = otter_target_create(
      "otter_logger.o", allocator, filesystem, logger, "otter_logger.c",
      "otter_logger.h", "otter_allocator.h", "otter_inc.h", NULL);
  otter_target_add_command(otter_logger_obj,
                           "cc -c otter_logger.c -o otter_logger.o " CC_FLAGS);

  OTTER_CLEANUP(otter_target_free_p)
  otter_target *otter_file_obj =
      otter_target_create("otter_file.o", allocator, filesystem, logger,
                          "otter_file.c", "otter_file.h", NULL);
  otter_target_add_command(otter_file_obj,
                           "cc -c otter_file.c -o otter_file.o " CC_FLAGS);

  OTTER_CLEANUP(otter_target_free_p)
  otter_target *otter_filesystem_obj = otter_target_create(
      "otter_filesystem.o", allocator, filesystem, logger, "otter_filesystem.c",
      "otter_filesystem.h", "otter_file.h", "otter_allocator.h", "otter_inc.h",
      NULL);
  otter_target_add_command(
      otter_filesystem_obj,
      "cc -c otter_filesystem.c -o otter_filesystem.o " CC_FLAGS);

  OTTER_CLEANUP(otter_target_free_p)
  otter_target *otter_cstring_obj = otter_target_create(
      "otter_cstring.o", allocator, filesystem, logger, "otter_cstring.c",
      "otter_cstring.h", "otter_allocator.h", NULL);
  otter_target_add_command(
      otter_cstring_obj,
      "cc -c -fPIC otter_cstring.c -o otter_cstring.o " CC_FLAGS);

  OTTER_CLEANUP(otter_target_free_p)
  otter_target *otter_target_obj = otter_target_create(
      "otter_target.o", allocator, filesystem, logger, "otter_target.c",
      "otter_target.h", "otter_allocator.h", "otter_filesystem.h",
      "otter_logger.h", "otter_cstring.h", "otter_inc.h", NULL);
  otter_target_add_command(otter_target_obj,
                           "cc -c otter_target.c -o otter_target.o " CC_FLAGS);

  OTTER_CLEANUP(otter_target_free_p)
  otter_target *otter_make_exe = otter_target_create(
      "otter_make", allocator, filesystem, logger, "otter_make.c",
      "otter_allocator.h", "otter_filesystem.h", "otter_logger.h",
      "otter_target.h", "otter_inc.h", NULL);
  otter_target_add_command(
      otter_make_exe,
      "cc otter_make.c otter_target.o otter_allocator.o otter_file.o "
      "otter_filesystem.o "
      "otter_cstring.o otter_logger.o -o otter_make -lgnutls " CC_FLAGS);
  otter_target_add_dependency(otter_make_exe, otter_target_obj);
  otter_target_add_dependency(otter_make_exe, otter_allocator_obj);
  otter_target_add_dependency(otter_make_exe, otter_cstring_obj);
  otter_target_add_dependency(otter_make_exe, otter_logger_obj);
  otter_target_add_dependency(otter_make_exe, otter_filesystem_obj);
  otter_target_add_dependency(otter_make_exe, otter_file_obj);

  otter_target_execute(otter_make_exe);

  /* Build the actual program */
  OTTER_CLEANUP(otter_target_free_p)
  otter_target *otter_token_obj = otter_target_create(
      "otter_token.o", allocator, filesystem, logger, "otter_token.c",
      "otter_token.h", "otter_allocator.h", NULL);
  otter_target_add_command(
      otter_token_obj, "cc -fPIC -c otter_token.c -o otter_token.o " CC_FLAGS);

  OTTER_CLEANUP(otter_target_free_p)
  otter_target *otter_lexer_obj = otter_target_create(
      "otter_lexer.o", allocator, filesystem, logger, "otter_lexer.c",
      "otter_lexer.h", "otter_allocator.h", "otter_token.h", "otter_array.h",
      "otter_inc.h", "otter_cstring.h", NULL);
  otter_target_add_command(
      otter_lexer_obj, "cc -fPIC -c otter_lexer.c -o otter_lexer.o " CC_FLAGS);

  /* Build tests */
  OTTER_CLEANUP(otter_target_free_p)
  otter_target *otter_test_obj = otter_target_create(
      "otter_test.o", allocator, filesystem, logger, "otter_test.c",
      "otter_test.h", "otter_allocator.h", NULL);
  otter_target_add_command(
      otter_test_obj, "cc -c -fPIC otter_test.c -o otter_test.o " CC_FLAGS);

  OTTER_CLEANUP(otter_target_free_p)
  otter_target *otter_test_driver = otter_target_create(
      "otter_test", allocator, filesystem, logger, "otter_test_driver.c",
      "otter_test.h", "otter_allocator.h", "otter_term_colors.h", NULL);
  otter_target_add_command(
      otter_test_driver,
      "cc -o otter_test otter_test_driver.c otter_allocator.o " CC_FLAGS);
  otter_target_add_dependency(otter_test_driver, otter_test_obj);
  otter_target_add_dependency(otter_test_driver, otter_allocator_obj);
  otter_target_execute(otter_test_driver);

  OTTER_CLEANUP(otter_target_free_p)
  otter_target *otter_cstring_tests = otter_target_create(
      "otter_cstring_tests.so", allocator, filesystem, logger,
      "otter_cstring_tests.c", "otter_test.h", NULL);
  otter_target_add_command(otter_cstring_tests,
                           "cc -fPIC -shared -o otter_cstring_tests.so "
                           "otter_cstring_tests.c otter_test.o otter_cstring.o "
                           "otter_allocator.o " CC_FLAGS);
  otter_target_add_dependency(otter_cstring_tests, otter_test_obj);
  otter_target_add_dependency(otter_cstring_tests, otter_cstring_obj);
  otter_target_add_dependency(otter_cstring_tests, otter_allocator_obj);
  otter_target_execute(otter_cstring_tests);

  OTTER_CLEANUP(otter_target_free_p)
  otter_target *otter_array_tests = otter_target_create(
      "otter_array_tests.so", allocator, filesystem, logger,
      "otter_array_tests.c", "otter_array.h", "otter_test.h", NULL);
  otter_target_add_command(
      otter_array_tests,
      "cc -fPIC -shared -o otter_array_tests.so "
      "otter_array_tests.c otter_test.o otter_allocator.o " CC_FLAGS);
  otter_target_add_dependency(otter_array_tests, otter_test_obj);
  otter_target_add_dependency(otter_array_tests, otter_allocator_obj);
  otter_target_execute(otter_array_tests);

  OTTER_CLEANUP(otter_target_free_p)
  otter_target *otter_lexer_tests = otter_target_create(
      "otter_lexer_tests.so", allocator, filesystem, logger,
      "otter_lexer_tests.c", "otter_test.h", "otter_lexer.h", NULL);
  otter_target_add_command(otter_lexer_tests,
                           "cc -fPIC -shared -o otter_lexer_tests.so "
                           "otter_lexer_tests.c otter_test.o otter_cstring.o "
                           "otter_token.o otter_lexer.o "
                           "otter_allocator.o " CC_FLAGS);
  otter_target_add_dependency(otter_lexer_tests, otter_test_obj);
  otter_target_add_dependency(otter_lexer_tests, otter_cstring_obj);
  otter_target_add_dependency(otter_lexer_tests, otter_lexer_obj);
  otter_target_add_dependency(otter_lexer_tests, otter_allocator_obj);
  otter_target_add_dependency(otter_lexer_tests, otter_token_obj);
  otter_target_execute(otter_lexer_tests);
  return 0;
}
