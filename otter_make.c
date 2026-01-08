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

#define CC_FLAGS_COMMON                                                        \
  "-std=gnu23 -Wall -Wextra -Werror -Wshadow -Wconversion "

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
  otter_target *otter_allocator_obj = otter_target_create_c_object(
      "otter_allocator.o", CC_FLAGS, allocator, filesystem, logger,
      "otter_allocator.c", NULL);

  OTTER_CLEANUP(otter_target_free_p)
  otter_target *otter_array_obj =
      otter_target_create_c_object("otter_array.o", CC_FLAGS, allocator,
                                   filesystem, logger, "otter_array.c", NULL);
  otter_target_add_dependency(otter_array_obj, otter_allocator_obj);

  OTTER_CLEANUP(otter_target_free_p)
  otter_target *otter_cstring_obj =
      otter_target_create_c_object("otter_cstring.o", CC_FLAGS, allocator,
                                   filesystem, logger, "otter_cstring.c", NULL);
  otter_target_add_dependency(otter_cstring_obj, otter_allocator_obj);

  OTTER_CLEANUP(otter_target_free_p)
  otter_target *otter_logger_obj =
      otter_target_create_c_object("otter_logger.o", CC_FLAGS, allocator,
                                   filesystem, logger, "otter_logger.c", NULL);
  otter_target_add_dependency(otter_logger_obj, otter_cstring_obj);
  otter_target_add_dependency(otter_logger_obj, otter_array_obj);
  otter_target_add_dependency(otter_logger_obj, otter_allocator_obj);

  OTTER_CLEANUP(otter_target_free_p)
  otter_target *otter_file_obj =
      otter_target_create_c_object("otter_file.o", CC_FLAGS, allocator,
                                   filesystem, logger, "otter_file.c", NULL);

  OTTER_CLEANUP(otter_target_free_p)
  otter_target *otter_filesystem_obj = otter_target_create_c_object(
      "otter_filesystem.o", CC_FLAGS, allocator, filesystem, logger,
      "otter_filesystem.c", NULL);
  otter_target_add_dependency(otter_filesystem_obj, otter_file_obj);
  otter_target_add_dependency(otter_filesystem_obj, otter_allocator_obj);

  OTTER_CLEANUP(otter_target_free_p)
  otter_target *otter_target_obj =
      otter_target_create_c_object("otter_target.o", CC_FLAGS, allocator,
                                   filesystem, logger, "otter_target.c", NULL);
  otter_target_add_dependency(otter_target_obj, otter_allocator_obj);
  otter_target_add_dependency(otter_target_obj, otter_array_obj);
  otter_target_add_dependency(otter_target_obj, otter_filesystem_obj);
  otter_target_add_dependency(otter_target_obj, otter_logger_obj);

  OTTER_CLEANUP(otter_target_free_p)
  otter_target *otter_make_exe = otter_target_create_c_executable(
      "otter_make", CC_FLAGS " -lgnutls", allocator, filesystem, logger,
      (const char *[]){"otter_make.c", NULL},
      (otter_target *[]){otter_allocator_obj, otter_filesystem_obj,
                         otter_logger_obj, otter_target_obj, NULL});
  otter_target_execute(otter_make_exe);

  /* Build the actual program */
  OTTER_CLEANUP(otter_target_free_p)
  otter_target *otter_token_obj =
      otter_target_create_c_object("otter_token.o", CC_FLAGS, allocator,
                                   filesystem, logger, "otter_token.c", NULL);
  otter_target_add_dependency(otter_token_obj, otter_allocator_obj);

  OTTER_CLEANUP(otter_target_free_p)
  otter_target *otter_node_obj =
      otter_target_create_c_object("otter_node.o", CC_FLAGS, allocator,
                                   filesystem, logger, "otter_node.c", NULL);
  otter_target_add_dependency(otter_node_obj, otter_allocator_obj);
  otter_target_add_dependency(otter_node_obj, otter_array_obj);

  OTTER_CLEANUP(otter_target_free_p)
  otter_target *otter_lexer_obj =
      otter_target_create_c_object("otter_lexer.o", CC_FLAGS, allocator,
                                   filesystem, logger, "otter_lexer.c", NULL);
  otter_target_add_dependency(otter_lexer_obj, otter_array_obj);
  otter_target_add_dependency(otter_lexer_obj, otter_cstring_obj);

  OTTER_CLEANUP(otter_target_free_p)
  otter_target *otter_parser_obj =
      otter_target_create_c_object("otter_parser.o", CC_FLAGS, allocator,
                                   filesystem, logger, "otter_parser.c", NULL);
  otter_target_add_dependency(otter_parser_obj, otter_allocator_obj);
  otter_target_add_dependency(otter_parser_obj, otter_logger_obj);
  otter_target_add_dependency(otter_parser_obj, otter_node_obj);
  otter_target_add_dependency(otter_parser_obj, otter_cstring_obj);

  OTTER_CLEANUP(otter_target_free_p)
  otter_target *otter_bytecode_obj = otter_target_create_c_object(
      "otter_bytecode.o", CC_FLAGS, allocator, filesystem, logger,
      "otter_bytecode.c", NULL);

  OTTER_CLEANUP(otter_target_free_p)
  otter_target *otter_vm_obj =
      otter_target_create_c_object("otter_vm.o", CC_FLAGS, allocator,
                                   filesystem, logger, "otter_vm.c", NULL);
  otter_target_add_dependency(otter_vm_obj, otter_allocator_obj);
  otter_target_add_dependency(otter_vm_obj, otter_logger_obj);
  otter_target_add_dependency(otter_vm_obj, otter_bytecode_obj);

  OTTER_CLEANUP(otter_target_free_p)
  otter_target *otter_exe = otter_target_create_c_executable(
      "otter", CC_FLAGS, allocator, filesystem, logger,
      (const char *[]){"otter.c", NULL},
      (otter_target *[]){otter_vm_obj, NULL});
  otter_target_execute(otter_exe);

  /* Build tests */
  OTTER_CLEANUP(otter_target_free_p)
  otter_target *otter_test_obj =
      otter_target_create_c_object("otter_test.o", CC_FLAGS, allocator,
                                   filesystem, logger, "otter_test.c", NULL);
  otter_target_add_dependency(otter_test_obj, otter_allocator_obj);

  OTTER_CLEANUP(otter_target_free_p)
  otter_target *otter_test_driver = otter_target_create_c_executable(
      "otter_test", CC_FLAGS, allocator, filesystem, logger,
      (const char *[]){"otter_test_driver.c", NULL},
      (otter_target *[]){otter_allocator_obj, NULL});
  otter_target_execute(otter_test_driver);

  OTTER_CLEANUP(otter_target_free_p)
  otter_target *otter_cstring_tests = otter_target_create_c_shared_object(
      "otter_cstring_tests.so", CC_FLAGS, allocator, filesystem, logger,
      (const char *[]){"otter_cstring_tests.c", NULL},
      (otter_target *[]){otter_test_obj, otter_cstring_obj, NULL});
  otter_target_execute(otter_cstring_tests);

  OTTER_CLEANUP(otter_target_free_p)
  otter_target *otter_array_tests = otter_target_create_c_shared_object(
      "otter_array_tests.so", CC_FLAGS, allocator, filesystem, logger,
      (const char *[]){"otter_array_tests.c", NULL},
      (otter_target *[]){otter_test_obj, otter_array_obj, NULL});
  otter_target_execute(otter_array_tests);

  OTTER_CLEANUP(otter_target_free_p)
  otter_target *otter_lexer_tests = otter_target_create_c_shared_object(
      "otter_lexer_tests.so", CC_FLAGS, allocator, filesystem, logger,
      (const char *[]){"otter_lexer_tests.c", NULL},
      (otter_target *[]){otter_test_obj, otter_lexer_obj, otter_token_obj,
                         NULL});
  otter_target_execute(otter_lexer_tests);

  OTTER_CLEANUP(otter_target_free_p)
  otter_target *otter_parser_tests = otter_target_create_c_shared_object(
      "otter_parser_tests.so", CC_FLAGS, allocator, filesystem, logger,
      (const char *[]){"otter_parser_tests.c", NULL},
      (otter_target *[]){otter_test_obj, otter_cstring_obj, otter_node_obj,
                         otter_parser_obj, NULL});
  otter_target_execute(otter_parser_tests);

  OTTER_CLEANUP(otter_target_free_p)
  otter_target *otter_parser_integration_tests =
      otter_target_create_c_shared_object(
          "otter_parser_integration_tests.so", CC_FLAGS, allocator, filesystem,
          logger, (const char *[]){"otter_parser_integration_tests.c", NULL},
          (otter_target *[]){otter_test_obj, otter_lexer_obj, otter_node_obj,
                             otter_parser_obj, NULL});
  otter_target_execute(otter_parser_integration_tests);
  return 0;
}
