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
#include "otter_logger.h"
#include "otter_target.h"

#include <stddef.h>

#define CC_FLAGS                                                               \
  "-std=c23 -g -Wall -Wextra -Werror -Wpedantic -Wshadow -Wconversion "        \
  "-fsanitize=address,undefined"

void build_tests(otter_allocator *allocator, otter_filesystem *filesystem,
                 otter_logger *logger) {
  otter_target *otter_test_driver =
      otter_target_create("otter_test", allocator, filesystem, logger,
                          "otter_test_driver.c", "otter_test.h", NULL);
  otter_target_add_command(otter_test_driver,
                           "cc -o otter_test otter_test_driver.c " CC_FLAGS);
  otter_target_execute(otter_test_driver);

  otter_target *otter_cstring_tests = otter_target_create(
      "otter_cstring_tests.so", allocator, filesystem, logger,
      "otter_cstring_tests.c", "otter_test.h", NULL);
  otter_target_add_command(otter_cstring_tests,
                           "cc -fPIC -shared -o otter_cstring_tests.so "
                           "otter_cstring_tests.c " CC_FLAGS);
  otter_target_execute(otter_cstring_tests);

  otter_target_free(otter_test_driver);
  otter_target_free(otter_cstring_tests);
}

int main() {
  otter_allocator *allocator = otter_allocator_create();
  otter_logger *logger = otter_logger_create(allocator, OTTER_LOG_LEVEL_INFO);
  otter_logger_add_sink(logger, otter_logger_console_sink);
  otter_filesystem *filesystem = otter_filesystem_create(allocator);

  /* Build the build system */
  otter_target *otter_allocator_obj =
      otter_target_create("otter_allocator.o", allocator, filesystem, logger,
                          "otter_allocator.c", "otter_allocator.h", NULL);
  otter_target_add_command(
      otter_allocator_obj,
      "cc -c otter_allocator.c -o otter_allocator.o " CC_FLAGS);

  otter_target *otter_logger_obj = otter_target_create(
      "otter_logger.o", allocator, filesystem, logger, "otter_logger.c",
      "otter_logger.h", "otter_allocator.h", NULL);
  otter_target_add_command(otter_logger_obj,
                           "cc -c otter_logger.c -o otter_logger.o " CC_FLAGS);

  otter_target *otter_file_obj =
      otter_target_create("otter_file.o", allocator, filesystem, logger,
                          "otter_file.c", "otter_file.h", NULL);
  otter_target_add_command(otter_file_obj,
                           "cc -c otter_file.c -o otter_file.o " CC_FLAGS);

  otter_target *otter_filesystem_obj = otter_target_create(
      "otter_filesystem.o", allocator, filesystem, logger, "otter_filesystem.c",
      "otter_filesystem.h", "otter_file.h", "otter_allocator.h", NULL);
  otter_target_add_command(
      otter_filesystem_obj,
      "cc -c otter_filesystem.c -o otter_filesystem.o " CC_FLAGS);

  otter_target *otter_cstring_obj = otter_target_create(
      "otter_cstring.o", allocator, filesystem, logger, "otter_cstring.c",
      "otter_cstring.h", "otter_allocator.h", NULL);
  otter_target_add_command(
      otter_cstring_obj, "cc -c otter_cstring.c -o otter_cstring.o " CC_FLAGS);

  otter_target *otter_target_obj = otter_target_create(
      "otter_target.o", allocator, filesystem, logger, "otter_target.c",
      "otter_target.h", "otter_allocator.h", "otter_filesystem.h",
      "otter_logger.h", "otter_cstring.h", NULL);
  otter_target_add_command(otter_target_obj,
                           "cc -c otter_target.c -o otter_target.o " CC_FLAGS);

  otter_target *otter_make_exe = otter_target_create(
      "otter_make", allocator, filesystem, logger, "otter_make.c", NULL);
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

  build_tests(allocator, filesystem, logger);

  /* Build the actual program */

  otter_target *otter_lexer_obj = otter_target_create(
      "otter_lexer.o", allocator, filesystem, logger, "otter_lexer.c",
      "otter_lexer.h", "otter_allocator.h", NULL);
  otter_target_add_command(otter_lexer_obj,
                           "cc -c otter_lexer.c -o otter_lexer.o " CC_FLAGS);

  otter_target_execute(otter_lexer_obj);

  otter_target_free(otter_allocator_obj);
  otter_target_free(otter_logger_obj);
  otter_target_free(otter_cstring_obj);
  otter_target_free(otter_target_obj);
  otter_target_free(otter_file_obj);
  otter_target_free(otter_filesystem_obj);
  otter_target_free(otter_make_exe);

  otter_target_free(otter_lexer_obj);

  otter_logger_free(logger);
  otter_allocator_free(allocator);
  return 0;
}
