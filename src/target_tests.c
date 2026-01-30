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

static void *malloc_mock_fail(otter_allocator * /* unused */,
                              size_t /*unused */) {
  return NULL;
}

OTTER_TEST(target_create_c_object_malloc_fails) {
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

  /* Create failing allocator */
  otter_allocator_vtable vtable = {
      .malloc = malloc_mock_fail,
      .realloc = OTTER_TEST_ALLOCATOR->vtable->realloc,
      .free = OTTER_TEST_ALLOCATOR->vtable->free,
  };
  otter_allocator allocator = {
      .vtable = &vtable,
  };

  /* Target creation should fail due to malloc failure */
  target =
      otter_target_create_c_object(name, flags, include_flags, &allocator,
                                   filesystem, logger, proc_mgr, file, NULL);
  OTTER_ASSERT(target == NULL);

  OTTER_TEST_END(if (proc_mgr) otter_process_manager_free(proc_mgr);
                 if (logger) otter_logger_free(logger);
                 if (filesystem) otter_filesystem_free(filesystem);
                 if (name) otter_string_free(name);
                 if (flags) otter_string_free(flags);
                 if (include_flags) otter_string_free(include_flags);
                 if (file) otter_string_free(file););
}

OTTER_TEST(target_create_executable_malloc_fails) {
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

  flags = otter_string_from_cstr(OTTER_TEST_ALLOCATOR, "-Wall");
  OTTER_ASSERT(flags != NULL);

  include_flags = otter_string_from_cstr(OTTER_TEST_ALLOCATOR, "-Iinclude");
  OTTER_ASSERT(include_flags != NULL);

  file = otter_string_from_cstr(OTTER_TEST_ALLOCATOR, "test_fixtures/main.c");
  OTTER_ASSERT(file != NULL);

  /* Create failing allocator */
  otter_allocator_vtable vtable = {
      .malloc = malloc_mock_fail,
      .realloc = OTTER_TEST_ALLOCATOR->vtable->realloc,
      .free = OTTER_TEST_ALLOCATOR->vtable->free,
  };
  otter_allocator allocator = {
      .vtable = &vtable,
  };

  const otter_string *files[] = {file, NULL};
  otter_target *deps[] = {NULL};

  /* Target creation should fail due to malloc failure */
  target = otter_target_create_c_executable(name, flags, include_flags,
                                            &allocator, filesystem, logger,
                                            proc_mgr, files, deps);
  OTTER_ASSERT(target == NULL);

  OTTER_TEST_END(if (proc_mgr) otter_process_manager_free(proc_mgr);
                 if (logger) otter_logger_free(logger);
                 if (filesystem) otter_filesystem_free(filesystem);
                 if (name) otter_string_free(name);
                 if (flags) otter_string_free(flags);
                 if (include_flags) otter_string_free(include_flags);
                 if (file) otter_string_free(file););
}

OTTER_TEST(target_create_shared_object_malloc_fails) {
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

  /* Create failing allocator */
  otter_allocator_vtable vtable = {
      .malloc = malloc_mock_fail,
      .realloc = OTTER_TEST_ALLOCATOR->vtable->realloc,
      .free = OTTER_TEST_ALLOCATOR->vtable->free,
  };
  otter_allocator allocator = {
      .vtable = &vtable,
  };

  const otter_string *files[] = {file, NULL};
  otter_target *deps[] = {NULL};

  /* Target creation should fail due to malloc failure */
  target = otter_target_create_c_shared_object(name, flags, include_flags,
                                               &allocator, filesystem, logger,
                                               proc_mgr, files, deps);
  OTTER_ASSERT(target == NULL);

  OTTER_TEST_END(if (proc_mgr) otter_process_manager_free(proc_mgr);
                 if (logger) otter_logger_free(logger);
                 if (filesystem) otter_filesystem_free(filesystem);
                 if (name) otter_string_free(name);
                 if (flags) otter_string_free(flags);
                 if (include_flags) otter_string_free(include_flags);
                 if (file) otter_string_free(file););
}

/* Counter for selective malloc failures */
static int malloc_call_count = 0;
static int malloc_fail_at = -1;
static otter_allocator *saved_allocator = NULL;

/* Named constants for malloc call positions */
#define NAME_COPY_MALLOC_COUNT 2
#define DEPENDENCIES_ARRAY_MALLOC_COUNT 3
#define FILES_ARRAY_MALLOC_COUNT 4
#define CC_FLAGS_COPY_MALLOC_COUNT 5
#define INCLUDE_FLAGS_COPY_MALLOC_COUNT 7
#define FILE_COPY_MALLOC_COUNT 9

static void *malloc_mock_selective(otter_allocator *allocator, size_t size) {
  malloc_call_count++;
  if (malloc_call_count == malloc_fail_at) {
    return NULL;
  }
  return saved_allocator->vtable->malloc(allocator, size);
}

OTTER_TEST(target_create_name_copy_fails) {
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

  /* Create allocator that fails on 2nd malloc (name copy) */
  malloc_call_count = 0;
  malloc_fail_at = NAME_COPY_MALLOC_COUNT;
  saved_allocator = OTTER_TEST_ALLOCATOR;

  otter_allocator_vtable vtable = {
      .malloc = malloc_mock_selective,
      .realloc = OTTER_TEST_ALLOCATOR->vtable->realloc,
      .free = OTTER_TEST_ALLOCATOR->vtable->free,
  };
  otter_allocator allocator = {
      .vtable = &vtable,
  };

  /* Target creation should fail when name copy fails */
  target =
      otter_target_create_c_object(name, flags, include_flags, &allocator,
                                   filesystem, logger, proc_mgr, file, NULL);
  OTTER_ASSERT(target == NULL);

  OTTER_TEST_END(if (proc_mgr) otter_process_manager_free(proc_mgr);
                 if (logger) otter_logger_free(logger);
                 if (filesystem) otter_filesystem_free(filesystem);
                 if (name) otter_string_free(name);
                 if (flags) otter_string_free(flags);
                 if (include_flags) otter_string_free(include_flags);
                 if (file) otter_string_free(file););
}

OTTER_TEST(target_create_dependencies_array_fails) {
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

  /* Create allocator that fails on 3rd malloc (dependencies array) */
  malloc_call_count = 0;
  malloc_fail_at = DEPENDENCIES_ARRAY_MALLOC_COUNT;
  saved_allocator = OTTER_TEST_ALLOCATOR;

  otter_allocator_vtable vtable = {
      .malloc = malloc_mock_selective,
      .realloc = OTTER_TEST_ALLOCATOR->vtable->realloc,
      .free = OTTER_TEST_ALLOCATOR->vtable->free,
  };
  otter_allocator allocator = {
      .vtable = &vtable,
  };

  /* Target creation should fail when dependencies array allocation fails */
  target =
      otter_target_create_c_object(name, flags, include_flags, &allocator,
                                   filesystem, logger, proc_mgr, file, NULL);
  OTTER_ASSERT(target == NULL);

  OTTER_TEST_END(if (proc_mgr) otter_process_manager_free(proc_mgr);
                 if (logger) otter_logger_free(logger);
                 if (filesystem) otter_filesystem_free(filesystem);
                 if (name) otter_string_free(name);
                 if (flags) otter_string_free(flags);
                 if (include_flags) otter_string_free(include_flags);
                 if (file) otter_string_free(file););
}

OTTER_TEST(target_create_files_array_fails) {
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

  /* Create allocator that fails on 4th malloc (files array) */
  malloc_call_count = 0;
  malloc_fail_at = FILES_ARRAY_MALLOC_COUNT;
  saved_allocator = OTTER_TEST_ALLOCATOR;

  otter_allocator_vtable vtable = {
      .malloc = malloc_mock_selective,
      .realloc = OTTER_TEST_ALLOCATOR->vtable->realloc,
      .free = OTTER_TEST_ALLOCATOR->vtable->free,
  };
  otter_allocator allocator = {
      .vtable = &vtable,
  };

  /* Target creation should fail when files array allocation fails */
  target =
      otter_target_create_c_object(name, flags, include_flags, &allocator,
                                   filesystem, logger, proc_mgr, file, NULL);
  OTTER_ASSERT(target == NULL);

  OTTER_TEST_END(if (proc_mgr) otter_process_manager_free(proc_mgr);
                 if (logger) otter_logger_free(logger);
                 if (filesystem) otter_filesystem_free(filesystem);
                 if (name) otter_string_free(name);
                 if (flags) otter_string_free(flags);
                 if (include_flags) otter_string_free(include_flags);
                 if (file) otter_string_free(file););
}
