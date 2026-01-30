/*
  otter Copyright (C) 2026 Nathaniel Wright

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
#include "otter/filesystem.h"
#include "otter/logger.h"
#include "otter/process_manager.h"
#include "otter/string.h"
#include "otter/target.h"
#include "otter/test.h"

OTTER_TEST(target_create_c_object_basic) {
  otter_filesystem *filesystem = NULL;
  otter_logger *logger = NULL;
  otter_process_manager *proc_mgr = NULL;
  otter_target *target = NULL;
  otter_string *name = NULL;
  otter_string *flags = NULL;
  otter_string *include_flags = NULL;
  otter_string *file = NULL;

  filesystem = otter_filesystem_create(OTTER_TEST_ALLOCATOR);
  OTTER_ASSERT(filesystem != NULL);

  logger = otter_logger_create(OTTER_TEST_ALLOCATOR, OTTER_LOG_LEVEL_ERROR);
  OTTER_ASSERT(logger != NULL);

  proc_mgr = otter_process_manager_create(OTTER_TEST_ALLOCATOR, logger);
  OTTER_ASSERT(proc_mgr != NULL);

  name = otter_string_from_cstr(OTTER_TEST_ALLOCATOR, "test.o");
  OTTER_ASSERT(name != NULL);

  flags = otter_string_from_cstr(OTTER_TEST_ALLOCATOR, "-Wall -O2");
  OTTER_ASSERT(flags != NULL);

  include_flags = otter_string_from_cstr(OTTER_TEST_ALLOCATOR, "-Iinclude");
  OTTER_ASSERT(include_flags != NULL);

  file = otter_string_from_cstr(OTTER_TEST_ALLOCATOR, "test_fixtures/test.c");
  OTTER_ASSERT(file != NULL);

  target = otter_target_create_c_object(name, flags, include_flags,
                                        OTTER_TEST_ALLOCATOR, filesystem,
                                        logger, proc_mgr, file, NULL);
  OTTER_ASSERT(target != NULL);
  OTTER_ASSERT(target->type == OTTER_TARGET_OBJECT);
  OTTER_ASSERT(target->name != NULL);
  OTTER_ASSERT(target->command != NULL);
  OTTER_ASSERT(target->executed == false);

  OTTER_TEST_END(if (target) otter_target_free(target);
                 if (proc_mgr) otter_process_manager_free(proc_mgr);
                 if (logger) otter_logger_free(logger);
                 if (filesystem) otter_filesystem_free(filesystem);
                 if (name) otter_string_free(name);
                 if (flags) otter_string_free(flags);
                 if (include_flags) otter_string_free(include_flags);
                 if (file) otter_string_free(file););
}

/* Test: Create C object target with NULL parameters */
OTTER_TEST(target_create_c_object_null_params) {
  otter_filesystem *filesystem = NULL;
  otter_logger *logger = NULL;
  otter_process_manager *proc_mgr = NULL;
  otter_target *target = NULL;
  otter_string *name = NULL;
  otter_string *flags = NULL;
  otter_string *include_flags = NULL;

  filesystem = otter_filesystem_create(OTTER_TEST_ALLOCATOR);
  OTTER_ASSERT(filesystem != NULL);

  logger = otter_logger_create(OTTER_TEST_ALLOCATOR, OTTER_LOG_LEVEL_ERROR);
  OTTER_ASSERT(logger != NULL);

  proc_mgr = otter_process_manager_create(OTTER_TEST_ALLOCATOR, logger);
  OTTER_ASSERT(proc_mgr != NULL);

  flags = otter_string_from_cstr(OTTER_TEST_ALLOCATOR, "-Wall");
  OTTER_ASSERT(flags != NULL);

  include_flags = otter_string_from_cstr(OTTER_TEST_ALLOCATOR, "-Iinclude");
  OTTER_ASSERT(include_flags != NULL);

  /* NULL name should fail */
  target = otter_target_create_c_object(NULL, flags, include_flags,
                                        OTTER_TEST_ALLOCATOR, filesystem,
                                        logger, proc_mgr, NULL);
  OTTER_ASSERT(target == NULL);

  name = otter_string_from_cstr(OTTER_TEST_ALLOCATOR, "test.o");
  OTTER_ASSERT(name != NULL);

  /* NULL allocator should fail */
  target = otter_target_create_c_object(name, flags, include_flags, NULL,
                                        filesystem, logger, proc_mgr, NULL);
  OTTER_ASSERT(target == NULL);

  /* NULL filesystem should fail */
  target = otter_target_create_c_object(name, flags, include_flags,
                                        OTTER_TEST_ALLOCATOR, NULL, logger,
                                        proc_mgr, NULL);
  OTTER_ASSERT(target == NULL);

  /* NULL logger should fail */
  target = otter_target_create_c_object(name, flags, include_flags,
                                        OTTER_TEST_ALLOCATOR, filesystem, NULL,
                                        proc_mgr, NULL);
  OTTER_ASSERT(target == NULL);

  /* NULL process_manager should fail */
  target = otter_target_create_c_object(name, flags, include_flags,
                                        OTTER_TEST_ALLOCATOR, filesystem,
                                        logger, NULL, NULL);
  OTTER_ASSERT(target == NULL);

  OTTER_TEST_END(if (proc_mgr) otter_process_manager_free(proc_mgr);
                 if (logger) otter_logger_free(logger);
                 if (filesystem) otter_filesystem_free(filesystem);
                 if (name) otter_string_free(name);
                 if (flags) otter_string_free(flags);
                 if (include_flags) otter_string_free(include_flags););
}

OTTER_TEST(target_create_c_executable_basic) {
  otter_filesystem *filesystem = NULL;
  otter_logger *logger = NULL;
  otter_process_manager *proc_mgr = NULL;
  otter_target *target = NULL;
  otter_string *name = NULL;
  otter_string *flags = NULL;
  otter_string *include_flags = NULL;
  otter_string *file = NULL;

  filesystem = otter_filesystem_create(OTTER_TEST_ALLOCATOR);
  OTTER_ASSERT(filesystem != NULL);

  logger = otter_logger_create(OTTER_TEST_ALLOCATOR, OTTER_LOG_LEVEL_ERROR);
  OTTER_ASSERT(logger != NULL);

  proc_mgr = otter_process_manager_create(OTTER_TEST_ALLOCATOR, logger);
  OTTER_ASSERT(proc_mgr != NULL);

  name = otter_string_from_cstr(OTTER_TEST_ALLOCATOR, "test_exe");
  OTTER_ASSERT(name != NULL);

  flags = otter_string_from_cstr(OTTER_TEST_ALLOCATOR, "-Wall -O2");
  OTTER_ASSERT(flags != NULL);

  include_flags = otter_string_from_cstr(OTTER_TEST_ALLOCATOR, "-Iinclude");
  OTTER_ASSERT(include_flags != NULL);

  file = otter_string_from_cstr(OTTER_TEST_ALLOCATOR, "test_fixtures/main.c");
  OTTER_ASSERT(file != NULL);

  const otter_string *files[] = {file, NULL};
  otter_target *deps[] = {NULL};

  target = otter_target_create_c_executable(name, flags, include_flags,
                                            OTTER_TEST_ALLOCATOR, filesystem,
                                            logger, proc_mgr, files, deps);
  OTTER_ASSERT(target != NULL);
  OTTER_ASSERT(target->type == OTTER_TARGET_EXECUTABLE);
  OTTER_ASSERT(target->name != NULL);
  OTTER_ASSERT(target->command != NULL);

  OTTER_TEST_END(if (target) otter_target_free(target);
                 if (proc_mgr) otter_process_manager_free(proc_mgr);
                 if (logger) otter_logger_free(logger);
                 if (filesystem) otter_filesystem_free(filesystem);
                 if (name) otter_string_free(name);
                 if (flags) otter_string_free(flags);
                 if (include_flags) otter_string_free(include_flags);
                 if (file) otter_string_free(file););
}

OTTER_TEST(target_create_c_shared_object_basic) {
  otter_filesystem *filesystem = NULL;
  otter_logger *logger = NULL;
  otter_process_manager *proc_mgr = NULL;
  otter_target *target = NULL;
  otter_string *name = NULL;
  otter_string *flags = NULL;
  otter_string *include_flags = NULL;
  otter_string *file = NULL;

  filesystem = otter_filesystem_create(OTTER_TEST_ALLOCATOR);
  OTTER_ASSERT(filesystem != NULL);

  logger = otter_logger_create(OTTER_TEST_ALLOCATOR, OTTER_LOG_LEVEL_ERROR);
  OTTER_ASSERT(logger != NULL);

  proc_mgr = otter_process_manager_create(OTTER_TEST_ALLOCATOR, logger);
  OTTER_ASSERT(proc_mgr != NULL);

  name = otter_string_from_cstr(OTTER_TEST_ALLOCATOR, "libtest.so");
  OTTER_ASSERT(name != NULL);

  flags = otter_string_from_cstr(OTTER_TEST_ALLOCATOR, "-Wall -O2");
  OTTER_ASSERT(flags != NULL);

  include_flags = otter_string_from_cstr(OTTER_TEST_ALLOCATOR, "-Iinclude");
  OTTER_ASSERT(include_flags != NULL);

  file = otter_string_from_cstr(OTTER_TEST_ALLOCATOR, "test_fixtures/lib.c");
  OTTER_ASSERT(file != NULL);

  const otter_string *files[] = {file, NULL};
  otter_target *deps[] = {NULL};

  target = otter_target_create_c_shared_object(name, flags, include_flags,
                                               OTTER_TEST_ALLOCATOR, filesystem,
                                               logger, proc_mgr, files, deps);
  OTTER_ASSERT(target != NULL);
  OTTER_ASSERT(target->type == OTTER_TARGET_SHARED_OBJECT);
  OTTER_ASSERT(target->name != NULL);
  OTTER_ASSERT(target->command != NULL);

  OTTER_TEST_END(if (target) otter_target_free(target);
                 if (proc_mgr) otter_process_manager_free(proc_mgr);
                 if (logger) otter_logger_free(logger);
                 if (filesystem) otter_filesystem_free(filesystem);
                 if (name) otter_string_free(name);
                 if (flags) otter_string_free(flags);
                 if (include_flags) otter_string_free(include_flags);
                 if (file) otter_string_free(file););
}

OTTER_TEST(target_free_null) {
  /* Should not crash */
  otter_target_free(NULL);

  /* Add a trivial assertion to use the label */
  OTTER_ASSERT(true);

  OTTER_TEST_END();
}

OTTER_TEST(target_add_command) {
  otter_filesystem *filesystem = NULL;
  otter_logger *logger = NULL;
  otter_process_manager *proc_mgr = NULL;
  otter_target *target = NULL;
  otter_string *name = NULL;
  otter_string *flags = NULL;
  otter_string *include_flags = NULL;
  otter_string *file = NULL;
  otter_string *command = NULL;

  filesystem = otter_filesystem_create(OTTER_TEST_ALLOCATOR);
  OTTER_ASSERT(filesystem != NULL);

  logger = otter_logger_create(OTTER_TEST_ALLOCATOR, OTTER_LOG_LEVEL_ERROR);
  OTTER_ASSERT(logger != NULL);

  proc_mgr = otter_process_manager_create(OTTER_TEST_ALLOCATOR, logger);
  OTTER_ASSERT(proc_mgr != NULL);

  name = otter_string_from_cstr(OTTER_TEST_ALLOCATOR, "test.o");
  OTTER_ASSERT(name != NULL);

  flags = otter_string_from_cstr(OTTER_TEST_ALLOCATOR, "-Wall");
  OTTER_ASSERT(flags != NULL);

  include_flags = otter_string_from_cstr(OTTER_TEST_ALLOCATOR, "-Iinclude");
  OTTER_ASSERT(include_flags != NULL);

  file = otter_string_from_cstr(OTTER_TEST_ALLOCATOR, "test_fixtures/test.c");
  OTTER_ASSERT(file != NULL);

  target = otter_target_create_c_object(name, flags, include_flags,
                                        OTTER_TEST_ALLOCATOR, filesystem,
                                        logger, proc_mgr, file, NULL);
  OTTER_ASSERT(target != NULL);

  command = otter_string_from_cstr(OTTER_TEST_ALLOCATOR, "echo test");
  OTTER_ASSERT(command != NULL);

  otter_target_add_command(target, command);
  OTTER_ASSERT(target->command != NULL);

  OTTER_TEST_END(if (target) otter_target_free(target);
                 if (proc_mgr) otter_process_manager_free(proc_mgr);
                 if (logger) otter_logger_free(logger);
                 if (filesystem) otter_filesystem_free(filesystem);
                 if (name) otter_string_free(name);
                 if (flags) otter_string_free(flags);
                 if (include_flags) otter_string_free(include_flags);
                 if (file) otter_string_free(file);
                 if (command) otter_string_free(command););
}

OTTER_TEST(target_add_command_null_target) {
  otter_string *command = NULL;

  command = otter_string_from_cstr(OTTER_TEST_ALLOCATOR, "echo test");
  OTTER_ASSERT(command != NULL);

  /* Should not crash */
  otter_target_add_command(NULL, command);

  OTTER_TEST_END(if (command) otter_string_free(command););
}

OTTER_TEST(target_add_command_null_command) {
  otter_filesystem *filesystem = NULL;
  otter_logger *logger = NULL;
  otter_process_manager *proc_mgr = NULL;
  otter_target *target = NULL;
  otter_string *name = NULL;
  otter_string *flags = NULL;
  otter_string *include_flags = NULL;
  otter_string *file = NULL;

  filesystem = otter_filesystem_create(OTTER_TEST_ALLOCATOR);
  OTTER_ASSERT(filesystem != NULL);

  logger = otter_logger_create(OTTER_TEST_ALLOCATOR, OTTER_LOG_LEVEL_ERROR);
  OTTER_ASSERT(logger != NULL);

  proc_mgr = otter_process_manager_create(OTTER_TEST_ALLOCATOR, logger);
  OTTER_ASSERT(proc_mgr != NULL);

  name = otter_string_from_cstr(OTTER_TEST_ALLOCATOR, "test.o");
  OTTER_ASSERT(name != NULL);

  flags = otter_string_from_cstr(OTTER_TEST_ALLOCATOR, "-Wall");
  OTTER_ASSERT(flags != NULL);

  include_flags = otter_string_from_cstr(OTTER_TEST_ALLOCATOR, "-Iinclude");
  OTTER_ASSERT(include_flags != NULL);

  file = otter_string_from_cstr(OTTER_TEST_ALLOCATOR, "test_fixtures/test.c");
  OTTER_ASSERT(file != NULL);

  target = otter_target_create_c_object(name, flags, include_flags,
                                        OTTER_TEST_ALLOCATOR, filesystem,
                                        logger, proc_mgr, file, NULL);
  OTTER_ASSERT(target != NULL);

  /* Should not crash */
  otter_target_add_command(target, NULL);

  OTTER_TEST_END(if (target) otter_target_free(target);
                 if (proc_mgr) otter_process_manager_free(proc_mgr);
                 if (logger) otter_logger_free(logger);
                 if (filesystem) otter_filesystem_free(filesystem);
                 if (name) otter_string_free(name);
                 if (flags) otter_string_free(flags);
                 if (include_flags) otter_string_free(include_flags);
                 if (file) otter_string_free(file););
}

OTTER_TEST(target_add_dependency) {
  otter_filesystem *filesystem = NULL;
  otter_logger *logger = NULL;
  otter_process_manager *proc_mgr = NULL;
  otter_target *target1 = NULL;
  otter_target *target2 = NULL;
  otter_string *name1 = NULL;
  otter_string *name2 = NULL;
  otter_string *flags = NULL;
  otter_string *include_flags = NULL;
  otter_string *file1 = NULL;
  otter_string *file2 = NULL;

  filesystem = otter_filesystem_create(OTTER_TEST_ALLOCATOR);
  OTTER_ASSERT(filesystem != NULL);

  logger = otter_logger_create(OTTER_TEST_ALLOCATOR, OTTER_LOG_LEVEL_ERROR);
  OTTER_ASSERT(logger != NULL);

  proc_mgr = otter_process_manager_create(OTTER_TEST_ALLOCATOR, logger);
  OTTER_ASSERT(proc_mgr != NULL);

  name1 = otter_string_from_cstr(OTTER_TEST_ALLOCATOR, "test1.o");
  OTTER_ASSERT(name1 != NULL);

  name2 = otter_string_from_cstr(OTTER_TEST_ALLOCATOR, "test2.o");
  OTTER_ASSERT(name2 != NULL);

  flags = otter_string_from_cstr(OTTER_TEST_ALLOCATOR, "-Wall");
  OTTER_ASSERT(flags != NULL);

  include_flags = otter_string_from_cstr(OTTER_TEST_ALLOCATOR, "-Iinclude");
  OTTER_ASSERT(include_flags != NULL);

  file1 = otter_string_from_cstr(OTTER_TEST_ALLOCATOR, "test_fixtures/test1.c");
  OTTER_ASSERT(file1 != NULL);

  file2 = otter_string_from_cstr(OTTER_TEST_ALLOCATOR, "test_fixtures/test2.c");
  OTTER_ASSERT(file2 != NULL);

  target1 = otter_target_create_c_object(name1, flags, include_flags,
                                         OTTER_TEST_ALLOCATOR, filesystem,
                                         logger, proc_mgr, file1, NULL);
  OTTER_ASSERT(target1 != NULL);

  target2 = otter_target_create_c_object(name2, flags, include_flags,
                                         OTTER_TEST_ALLOCATOR, filesystem,
                                         logger, proc_mgr, file2, NULL);
  OTTER_ASSERT(target2 != NULL);

  size_t initial_deps = OTTER_ARRAY_LENGTH(target1, dependencies);
  otter_target_add_dependency(target1, target2);
  OTTER_ASSERT(OTTER_ARRAY_LENGTH(target1, dependencies) == initial_deps + 1);

  OTTER_TEST_END(if (target1) otter_target_free(target1);
                 if (target2) otter_target_free(target2);
                 if (proc_mgr) otter_process_manager_free(proc_mgr);
                 if (logger) otter_logger_free(logger);
                 if (filesystem) otter_filesystem_free(filesystem);
                 if (name1) otter_string_free(name1);
                 if (name2) otter_string_free(name2);
                 if (flags) otter_string_free(flags);
                 if (include_flags) otter_string_free(include_flags);
                 if (file1) otter_string_free(file1);
                 if (file2) otter_string_free(file2););
}

OTTER_TEST(target_execute_null) {
  int result = otter_target_execute(NULL);
  OTTER_ASSERT(result == -1);

  OTTER_TEST_END();
}

OTTER_TEST(target_create_c_object_multiple_files) {
  otter_filesystem *filesystem = NULL;
  otter_logger *logger = NULL;
  otter_process_manager *proc_mgr = NULL;
  otter_target *target = NULL;
  otter_string *name = NULL;
  otter_string *flags = NULL;
  otter_string *include_flags = NULL;
  otter_string *file1 = NULL;
  otter_string *file2 = NULL;

  filesystem = otter_filesystem_create(OTTER_TEST_ALLOCATOR);
  OTTER_ASSERT(filesystem != NULL);

  logger = otter_logger_create(OTTER_TEST_ALLOCATOR, OTTER_LOG_LEVEL_ERROR);
  OTTER_ASSERT(logger != NULL);

  proc_mgr = otter_process_manager_create(OTTER_TEST_ALLOCATOR, logger);
  OTTER_ASSERT(proc_mgr != NULL);

  name = otter_string_from_cstr(OTTER_TEST_ALLOCATOR, "test.o");
  OTTER_ASSERT(name != NULL);

  flags = otter_string_from_cstr(OTTER_TEST_ALLOCATOR, "-Wall");
  OTTER_ASSERT(flags != NULL);

  include_flags = otter_string_from_cstr(OTTER_TEST_ALLOCATOR, "-Iinclude");
  OTTER_ASSERT(include_flags != NULL);

  file1 = otter_string_from_cstr(OTTER_TEST_ALLOCATOR, "test_fixtures/test1.c");
  OTTER_ASSERT(file1 != NULL);

  file2 = otter_string_from_cstr(OTTER_TEST_ALLOCATOR, "test_fixtures/test2.c");
  OTTER_ASSERT(file2 != NULL);

  target = otter_target_create_c_object(name, flags, include_flags,
                                        OTTER_TEST_ALLOCATOR, filesystem,
                                        logger, proc_mgr, file1, file2, NULL);
  OTTER_ASSERT(target != NULL);
  OTTER_ASSERT(OTTER_ARRAY_LENGTH(target, files) == 2);

  OTTER_TEST_END(if (target) otter_target_free(target);
                 if (proc_mgr) otter_process_manager_free(proc_mgr);
                 if (logger) otter_logger_free(logger);
                 if (filesystem) otter_filesystem_free(filesystem);
                 if (name) otter_string_free(name);
                 if (flags) otter_string_free(flags);
                 if (include_flags) otter_string_free(include_flags);
                 if (file1) otter_string_free(file1);
                 if (file2) otter_string_free(file2););
}

OTTER_TEST(target_create_executable_with_deps) {
  otter_filesystem *filesystem = NULL;
  otter_logger *logger = NULL;
  otter_process_manager *proc_mgr = NULL;
  otter_target *obj_target = NULL;
  otter_target *exe_target = NULL;
  otter_string *obj_name = NULL;
  otter_string *exe_name = NULL;
  otter_string *flags = NULL;
  otter_string *include_flags = NULL;
  otter_string *obj_file = NULL;
  otter_string *exe_file = NULL;

  filesystem = otter_filesystem_create(OTTER_TEST_ALLOCATOR);
  OTTER_ASSERT(filesystem != NULL);

  logger = otter_logger_create(OTTER_TEST_ALLOCATOR, OTTER_LOG_LEVEL_ERROR);
  OTTER_ASSERT(logger != NULL);

  proc_mgr = otter_process_manager_create(OTTER_TEST_ALLOCATOR, logger);
  OTTER_ASSERT(proc_mgr != NULL);

  obj_name = otter_string_from_cstr(OTTER_TEST_ALLOCATOR, "lib.o");
  OTTER_ASSERT(obj_name != NULL);

  exe_name = otter_string_from_cstr(OTTER_TEST_ALLOCATOR, "main");
  OTTER_ASSERT(exe_name != NULL);

  flags = otter_string_from_cstr(OTTER_TEST_ALLOCATOR, "-Wall");
  OTTER_ASSERT(flags != NULL);

  include_flags = otter_string_from_cstr(OTTER_TEST_ALLOCATOR, "-Iinclude");
  OTTER_ASSERT(include_flags != NULL);

  obj_file =
      otter_string_from_cstr(OTTER_TEST_ALLOCATOR, "test_fixtures/lib.c");
  OTTER_ASSERT(obj_file != NULL);

  exe_file =
      otter_string_from_cstr(OTTER_TEST_ALLOCATOR, "test_fixtures/main.c");
  OTTER_ASSERT(exe_file != NULL);

  obj_target = otter_target_create_c_object(obj_name, flags, include_flags,
                                            OTTER_TEST_ALLOCATOR, filesystem,
                                            logger, proc_mgr, obj_file, NULL);
  OTTER_ASSERT(obj_target != NULL);

  const otter_string *exe_files[] = {exe_file, NULL};
  otter_target *deps[] = {obj_target, NULL};

  exe_target = otter_target_create_c_executable(
      exe_name, flags, include_flags, OTTER_TEST_ALLOCATOR, filesystem, logger,
      proc_mgr, exe_files, deps);
  OTTER_ASSERT(exe_target != NULL);
  OTTER_ASSERT(OTTER_ARRAY_LENGTH(exe_target, dependencies) == 1);

  OTTER_TEST_END(if (exe_target) otter_target_free(exe_target);
                 if (obj_target) otter_target_free(obj_target);
                 if (proc_mgr) otter_process_manager_free(proc_mgr);
                 if (logger) otter_logger_free(logger);
                 if (filesystem) otter_filesystem_free(filesystem);
                 if (obj_name) otter_string_free(obj_name);
                 if (exe_name) otter_string_free(exe_name);
                 if (flags) otter_string_free(flags);
                 if (include_flags) otter_string_free(include_flags);
                 if (obj_file) otter_string_free(obj_file);
                 if (exe_file) otter_string_free(exe_file););
}

OTTER_TEST(target_create_shared_object_null_deps) {
  otter_filesystem *filesystem = NULL;
  otter_logger *logger = NULL;
  otter_process_manager *proc_mgr = NULL;
  otter_target *target = NULL;
  otter_string *name = NULL;
  otter_string *flags = NULL;
  otter_string *include_flags = NULL;
  otter_string *file = NULL;

  filesystem = otter_filesystem_create(OTTER_TEST_ALLOCATOR);
  OTTER_ASSERT(filesystem != NULL);

  logger = otter_logger_create(OTTER_TEST_ALLOCATOR, OTTER_LOG_LEVEL_ERROR);
  OTTER_ASSERT(logger != NULL);

  proc_mgr = otter_process_manager_create(OTTER_TEST_ALLOCATOR, logger);
  OTTER_ASSERT(proc_mgr != NULL);

  name = otter_string_from_cstr(OTTER_TEST_ALLOCATOR, "libtest.so");
  OTTER_ASSERT(name != NULL);

  flags = otter_string_from_cstr(OTTER_TEST_ALLOCATOR, "-Wall");
  OTTER_ASSERT(flags != NULL);

  include_flags = otter_string_from_cstr(OTTER_TEST_ALLOCATOR, "-Iinclude");
  OTTER_ASSERT(include_flags != NULL);

  file = otter_string_from_cstr(OTTER_TEST_ALLOCATOR, "test_fixtures/lib.c");
  OTTER_ASSERT(file != NULL);

  const otter_string *files[] = {file, NULL};

  /* NULL dependencies should fail */
  target = otter_target_create_c_shared_object(name, flags, include_flags,
                                               OTTER_TEST_ALLOCATOR, filesystem,
                                               logger, proc_mgr, files, NULL);
  OTTER_ASSERT(target == NULL);

  OTTER_TEST_END(if (proc_mgr) otter_process_manager_free(proc_mgr);
                 if (logger) otter_logger_free(logger);
                 if (filesystem) otter_filesystem_free(filesystem);
                 if (name) otter_string_free(name);
                 if (flags) otter_string_free(flags);
                 if (include_flags) otter_string_free(include_flags);
                 if (file) otter_string_free(file););
}

OTTER_TEST(target_execute_simple_object) {
  otter_filesystem *filesystem = NULL;
  otter_logger *logger = NULL;
  otter_process_manager *proc_mgr = NULL;
  otter_target *target = NULL;
  otter_string *name = NULL;
  otter_string *flags = NULL;
  otter_string *include_flags = NULL;
  otter_string *file = NULL;

  filesystem = otter_filesystem_create(OTTER_TEST_ALLOCATOR);
  OTTER_ASSERT(filesystem != NULL);

  logger = otter_logger_create(OTTER_TEST_ALLOCATOR, OTTER_LOG_LEVEL_ERROR);
  OTTER_ASSERT(logger != NULL);

  proc_mgr = otter_process_manager_create(OTTER_TEST_ALLOCATOR, logger);
  OTTER_ASSERT(proc_mgr != NULL);

  name = otter_string_from_cstr(OTTER_TEST_ALLOCATOR,
                                "test_fixtures/test_execute.o");
  OTTER_ASSERT(name != NULL);

  flags = otter_string_from_cstr(OTTER_TEST_ALLOCATOR, "-Wall");
  OTTER_ASSERT(flags != NULL);

  include_flags = otter_string_from_cstr(OTTER_TEST_ALLOCATOR, "-Iinclude");
  OTTER_ASSERT(include_flags != NULL);

  file = otter_string_from_cstr(OTTER_TEST_ALLOCATOR, "test_fixtures/test.c");
  OTTER_ASSERT(file != NULL);

  target = otter_target_create_c_object(name, flags, include_flags,
                                        OTTER_TEST_ALLOCATOR, filesystem,
                                        logger, proc_mgr, file, NULL);
  OTTER_ASSERT(target != NULL);

  /* Execute the target - this will compile the test file */
  int result = otter_target_execute(target);
  OTTER_ASSERT(result == 0);
  OTTER_ASSERT(target->executed == true);

  /* Clean up generated file */
  remove("test_fixtures/test_execute.o");

  OTTER_TEST_END(if (target) otter_target_free(target);
                 if (proc_mgr) otter_process_manager_free(proc_mgr);
                 if (logger) otter_logger_free(logger);
                 if (filesystem) otter_filesystem_free(filesystem);
                 if (name) otter_string_free(name);
                 if (flags) otter_string_free(flags);
                 if (include_flags) otter_string_free(include_flags);
                 if (file) otter_string_free(file););
}

OTTER_TEST(target_execute_already_up_to_date) {
  otter_filesystem *filesystem = NULL;
  otter_logger *logger = NULL;
  otter_process_manager *proc_mgr = NULL;
  otter_target *target = NULL;
  otter_string *name = NULL;
  otter_string *flags = NULL;
  otter_string *include_flags = NULL;
  otter_string *file = NULL;

  filesystem = otter_filesystem_create(OTTER_TEST_ALLOCATOR);
  OTTER_ASSERT(filesystem != NULL);

  logger = otter_logger_create(OTTER_TEST_ALLOCATOR, OTTER_LOG_LEVEL_ERROR);
  OTTER_ASSERT(logger != NULL);

  proc_mgr = otter_process_manager_create(OTTER_TEST_ALLOCATOR, logger);
  OTTER_ASSERT(proc_mgr != NULL);

  name = otter_string_from_cstr(OTTER_TEST_ALLOCATOR,
                                "test_fixtures/test_cached.o");
  OTTER_ASSERT(name != NULL);

  flags = otter_string_from_cstr(OTTER_TEST_ALLOCATOR, "-Wall");
  OTTER_ASSERT(flags != NULL);

  include_flags = otter_string_from_cstr(OTTER_TEST_ALLOCATOR, "-Iinclude");
  OTTER_ASSERT(include_flags != NULL);

  file = otter_string_from_cstr(OTTER_TEST_ALLOCATOR, "test_fixtures/test.c");
  OTTER_ASSERT(file != NULL);

  target = otter_target_create_c_object(name, flags, include_flags,
                                        OTTER_TEST_ALLOCATOR, filesystem,
                                        logger, proc_mgr, file, NULL);
  OTTER_ASSERT(target != NULL);

  /* First execution */
  int result = otter_target_execute(target);
  OTTER_ASSERT(result == 0);
  OTTER_ASSERT(target->executed == true);

  /* Free and recreate the target */
  otter_target_free(target);
  target = otter_target_create_c_object(name, flags, include_flags,
                                        OTTER_TEST_ALLOCATOR, filesystem,
                                        logger, proc_mgr, file, NULL);
  OTTER_ASSERT(target != NULL);

  /* Second execution - should be up-to-date */
  result = otter_target_execute(target);
  OTTER_ASSERT(result == 0);
  OTTER_ASSERT(target->executed == false);

  /* Clean up generated file */
  remove("test_fixtures/test_cached.o");

  OTTER_TEST_END(if (target) otter_target_free(target);
                 if (proc_mgr) otter_process_manager_free(proc_mgr);
                 if (logger) otter_logger_free(logger);
                 if (filesystem) otter_filesystem_free(filesystem);
                 if (name) otter_string_free(name);
                 if (flags) otter_string_free(flags);
                 if (include_flags) otter_string_free(include_flags);
                 if (file) otter_string_free(file););
}

OTTER_TEST(target_execute_with_dependencies) {
  otter_filesystem *filesystem = NULL;
  otter_logger *logger = NULL;
  otter_process_manager *proc_mgr = NULL;
  otter_target *obj_target = NULL;
  otter_target *exe_target = NULL;
  otter_string *obj_name = NULL;
  otter_string *exe_name = NULL;
  otter_string *flags = NULL;
  otter_string *include_flags = NULL;
  otter_string *obj_file = NULL;
  otter_string *exe_file = NULL;

  filesystem = otter_filesystem_create(OTTER_TEST_ALLOCATOR);
  OTTER_ASSERT(filesystem != NULL);

  logger = otter_logger_create(OTTER_TEST_ALLOCATOR, OTTER_LOG_LEVEL_ERROR);
  OTTER_ASSERT(logger != NULL);

  proc_mgr = otter_process_manager_create(OTTER_TEST_ALLOCATOR, logger);
  OTTER_ASSERT(proc_mgr != NULL);

  obj_name =
      otter_string_from_cstr(OTTER_TEST_ALLOCATOR, "test_fixtures/lib_exec.o");
  OTTER_ASSERT(obj_name != NULL);

  exe_name =
      otter_string_from_cstr(OTTER_TEST_ALLOCATOR, "test_fixtures/main_exec");
  OTTER_ASSERT(exe_name != NULL);

  flags = otter_string_from_cstr(OTTER_TEST_ALLOCATOR, "-Wall");
  OTTER_ASSERT(flags != NULL);

  include_flags = otter_string_from_cstr(OTTER_TEST_ALLOCATOR, "-Iinclude");
  OTTER_ASSERT(include_flags != NULL);

  obj_file =
      otter_string_from_cstr(OTTER_TEST_ALLOCATOR, "test_fixtures/lib.c");
  OTTER_ASSERT(obj_file != NULL);

  exe_file =
      otter_string_from_cstr(OTTER_TEST_ALLOCATOR, "test_fixtures/main.c");
  OTTER_ASSERT(exe_file != NULL);

  obj_target = otter_target_create_c_object(obj_name, flags, include_flags,
                                            OTTER_TEST_ALLOCATOR, filesystem,
                                            logger, proc_mgr, obj_file, NULL);
  OTTER_ASSERT(obj_target != NULL);

  const otter_string *exe_files[] = {exe_file, NULL};
  otter_target *deps[] = {obj_target, NULL};

  exe_target = otter_target_create_c_executable(
      exe_name, flags, include_flags, OTTER_TEST_ALLOCATOR, filesystem, logger,
      proc_mgr, exe_files, deps);
  OTTER_ASSERT(exe_target != NULL);

  /* Execute the executable - should also execute the dependency */
  int result = otter_target_execute(exe_target);
  OTTER_ASSERT(result == 0);
  OTTER_ASSERT(exe_target->executed == true);
  OTTER_ASSERT(obj_target->executed == true);

  /* Clean up generated files */
  remove("test_fixtures/lib_exec.o");
  remove("test_fixtures/main_exec");

  OTTER_TEST_END(if (exe_target) otter_target_free(exe_target);
                 if (obj_target) otter_target_free(obj_target);
                 if (proc_mgr) otter_process_manager_free(proc_mgr);
                 if (logger) otter_logger_free(logger);
                 if (filesystem) otter_filesystem_free(filesystem);
                 if (obj_name) otter_string_free(obj_name);
                 if (exe_name) otter_string_free(exe_name);
                 if (flags) otter_string_free(flags);
                 if (include_flags) otter_string_free(include_flags);
                 if (obj_file) otter_string_free(obj_file);
                 if (exe_file) otter_string_free(exe_file););
}

OTTER_TEST(target_create_object_empty_flags) {
  otter_filesystem *filesystem = NULL;
  otter_logger *logger = NULL;
  otter_process_manager *proc_mgr = NULL;
  otter_target *target = NULL;
  otter_string *name = NULL;
  otter_string *flags = NULL;
  otter_string *include_flags = NULL;
  otter_string *file = NULL;

  filesystem = otter_filesystem_create(OTTER_TEST_ALLOCATOR);
  OTTER_ASSERT(filesystem != NULL);

  logger = otter_logger_create(OTTER_TEST_ALLOCATOR, OTTER_LOG_LEVEL_ERROR);
  OTTER_ASSERT(logger != NULL);

  proc_mgr = otter_process_manager_create(OTTER_TEST_ALLOCATOR, logger);
  OTTER_ASSERT(proc_mgr != NULL);

  name = otter_string_from_cstr(OTTER_TEST_ALLOCATOR, "test_empty_flags.o");
  OTTER_ASSERT(name != NULL);

  flags = otter_string_from_cstr(OTTER_TEST_ALLOCATOR, "");
  OTTER_ASSERT(flags != NULL);

  include_flags = otter_string_from_cstr(OTTER_TEST_ALLOCATOR, "");
  OTTER_ASSERT(include_flags != NULL);

  file = otter_string_from_cstr(OTTER_TEST_ALLOCATOR, "test_fixtures/test.c");
  OTTER_ASSERT(file != NULL);

  target = otter_target_create_c_object(name, flags, include_flags,
                                        OTTER_TEST_ALLOCATOR, filesystem,
                                        logger, proc_mgr, file, NULL);
  OTTER_ASSERT(target != NULL);
  OTTER_ASSERT(target->command != NULL);

  OTTER_TEST_END(if (target) otter_target_free(target);
                 if (proc_mgr) otter_process_manager_free(proc_mgr);
                 if (logger) otter_logger_free(logger);
                 if (filesystem) otter_filesystem_free(filesystem);
                 if (name) otter_string_free(name);
                 if (flags) otter_string_free(flags);
                 if (include_flags) otter_string_free(include_flags);
                 if (file) otter_string_free(file););
}

OTTER_TEST(target_create_executable_null_files) {
  otter_filesystem *filesystem = NULL;
  otter_logger *logger = NULL;
  otter_process_manager *proc_mgr = NULL;
  otter_target *target = NULL;
  otter_string *name = NULL;
  otter_string *flags = NULL;
  otter_string *include_flags = NULL;

  filesystem = otter_filesystem_create(OTTER_TEST_ALLOCATOR);
  OTTER_ASSERT(filesystem != NULL);

  logger = otter_logger_create(OTTER_TEST_ALLOCATOR, OTTER_LOG_LEVEL_ERROR);
  OTTER_ASSERT(logger != NULL);

  proc_mgr = otter_process_manager_create(OTTER_TEST_ALLOCATOR, logger);
  OTTER_ASSERT(proc_mgr != NULL);

  name = otter_string_from_cstr(OTTER_TEST_ALLOCATOR, "test_exe");
  OTTER_ASSERT(name != NULL);

  flags = otter_string_from_cstr(OTTER_TEST_ALLOCATOR, "-Wall");
  OTTER_ASSERT(flags != NULL);

  include_flags = otter_string_from_cstr(OTTER_TEST_ALLOCATOR, "-Iinclude");
  OTTER_ASSERT(include_flags != NULL);

  otter_target *deps[] = {NULL};

  /* NULL files should fail */
  target = otter_target_create_c_executable(name, flags, include_flags,
                                            OTTER_TEST_ALLOCATOR, filesystem,
                                            logger, proc_mgr, NULL, deps);
  OTTER_ASSERT(target == NULL);

  OTTER_TEST_END(if (proc_mgr) otter_process_manager_free(proc_mgr);
                 if (logger) otter_logger_free(logger);
                 if (filesystem) otter_filesystem_free(filesystem);
                 if (name) otter_string_free(name);
                 if (flags) otter_string_free(flags);
                 if (include_flags) otter_string_free(include_flags););
}

OTTER_TEST(target_create_shared_object_null_files) {
  otter_filesystem *filesystem = NULL;
  otter_logger *logger = NULL;
  otter_process_manager *proc_mgr = NULL;
  otter_target *target = NULL;
  otter_string *name = NULL;
  otter_string *flags = NULL;
  otter_string *include_flags = NULL;

  filesystem = otter_filesystem_create(OTTER_TEST_ALLOCATOR);
  OTTER_ASSERT(filesystem != NULL);

  logger = otter_logger_create(OTTER_TEST_ALLOCATOR, OTTER_LOG_LEVEL_ERROR);
  OTTER_ASSERT(logger != NULL);

  proc_mgr = otter_process_manager_create(OTTER_TEST_ALLOCATOR, logger);
  OTTER_ASSERT(proc_mgr != NULL);

  name = otter_string_from_cstr(OTTER_TEST_ALLOCATOR, "libtest.so");
  OTTER_ASSERT(name != NULL);

  flags = otter_string_from_cstr(OTTER_TEST_ALLOCATOR, "-Wall");
  OTTER_ASSERT(flags != NULL);

  include_flags = otter_string_from_cstr(OTTER_TEST_ALLOCATOR, "-Iinclude");
  OTTER_ASSERT(include_flags != NULL);

  otter_target *deps[] = {NULL};

  /* NULL files should fail */
  target = otter_target_create_c_shared_object(name, flags, include_flags,
                                               OTTER_TEST_ALLOCATOR, filesystem,
                                               logger, proc_mgr, NULL, deps);
  OTTER_ASSERT(target == NULL);

  OTTER_TEST_END(if (proc_mgr) otter_process_manager_free(proc_mgr);
                 if (logger) otter_logger_free(logger);
                 if (filesystem) otter_filesystem_free(filesystem);
                 if (name) otter_string_free(name);
                 if (flags) otter_string_free(flags);
                 if (include_flags) otter_string_free(include_flags););
}
