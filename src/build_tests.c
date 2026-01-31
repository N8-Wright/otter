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
#include "otter/build.h"
#include "otter/filesystem.h"
#include "otter/logger.h"
#include "otter/process_manager.h"
#include "otter/test.h"

/* Dependency arrays - must be defined at file scope for static const use */
static const char *no_deps[] = {NULL};
static const char *target_b_deps[] = {"target_b", NULL};
static const char *target_a_deps[] = {"target_a", NULL};
static const char *target_c_deps[] = {"target_c", NULL};
static const char *self_deps[] = {"self_dep", NULL};
static const char *nonexistent_deps[] = {"nonexistent", NULL};

/* Test: Basic target creation with no dependencies */
OTTER_TEST(build_simple_object_target) {
  otter_filesystem *filesystem = NULL;
  otter_logger *logger = NULL;
  otter_process_manager *proc_mgr = NULL;
  otter_build_context *build_ctx = NULL;

  filesystem = otter_filesystem_create(OTTER_TEST_ALLOCATOR);
  OTTER_ASSERT(filesystem != NULL);

  logger = otter_logger_create(OTTER_TEST_ALLOCATOR, OTTER_LOG_LEVEL_ERROR);
  OTTER_ASSERT(logger != NULL);

  proc_mgr = otter_process_manager_create(OTTER_TEST_ALLOCATOR, logger);
  OTTER_ASSERT(proc_mgr != NULL);

  static const otter_target_definition targets[] = {
      OBJECT_TARGET("simple", no_deps), TARGET_LIST_END};

  otter_build_config config = {
      .paths = {.src_dir = "./test_src",
                .out_dir = "./test_out",
                .object_suffix = "",
                .shared_object_suffix = "",
                .executable_suffix = ""},
      .flags = {.cc_flags = "-Wall", .ll_flags = "", .include_flags = ""}};

  build_ctx = otter_build_context_create(targets, OTTER_TEST_ALLOCATOR,
                                         filesystem, logger, proc_mgr, &config);
  OTTER_ASSERT(build_ctx != NULL);

  OTTER_TEST_END(if (build_ctx) otter_build_context_free(build_ctx);
                 if (proc_mgr) otter_process_manager_free(proc_mgr);
                 if (logger) otter_logger_free(logger);
                 if (filesystem) otter_filesystem_free(filesystem););
}

/* Test: Simple circular dependency (A ↔ B) */
OTTER_TEST(build_detects_simple_circular_dependency) {
  otter_filesystem *filesystem = NULL;
  otter_logger *logger = NULL;
  otter_process_manager *proc_mgr = NULL;
  otter_build_context *build_ctx = NULL;

  filesystem = otter_filesystem_create(OTTER_TEST_ALLOCATOR);
  OTTER_ASSERT(filesystem != NULL);

  logger = otter_logger_create(OTTER_TEST_ALLOCATOR, OTTER_LOG_LEVEL_ERROR);
  OTTER_ASSERT(logger != NULL);

  proc_mgr = otter_process_manager_create(OTTER_TEST_ALLOCATOR, logger);
  OTTER_ASSERT(proc_mgr != NULL);

  /* A depends on B, B depends on A */
  static const otter_target_definition targets[] = {
      OBJECT_TARGET("target_a", target_b_deps),
      OBJECT_TARGET("target_b", target_a_deps), TARGET_LIST_END};

  otter_build_config config = {
      .paths = {.src_dir = "./test_src",
                .out_dir = "./test_out",
                .object_suffix = "",
                .shared_object_suffix = "",
                .executable_suffix = ""},
      .flags = {.cc_flags = "-Wall", .ll_flags = "", .include_flags = ""}};

  build_ctx = otter_build_context_create(targets, OTTER_TEST_ALLOCATOR,
                                         filesystem, logger, proc_mgr, &config);
  OTTER_ASSERT(build_ctx != NULL);

  /* Build should fail due to circular dependency */
  bool result = otter_build_all(build_ctx);
  OTTER_ASSERT(result == false);

  OTTER_TEST_END(if (build_ctx) otter_build_context_free(build_ctx);
                 if (proc_mgr) otter_process_manager_free(proc_mgr);
                 if (logger) otter_logger_free(logger);
                 if (filesystem) otter_filesystem_free(filesystem););
}

/* Test: Three-way circular dependency (A → B → C → A) */
OTTER_TEST(build_detects_three_way_circular_dependency) {
  otter_filesystem *filesystem = NULL;
  otter_logger *logger = NULL;
  otter_process_manager *proc_mgr = NULL;
  otter_build_context *build_ctx = NULL;

  filesystem = otter_filesystem_create(OTTER_TEST_ALLOCATOR);
  OTTER_ASSERT(filesystem != NULL);

  logger = otter_logger_create(OTTER_TEST_ALLOCATOR, OTTER_LOG_LEVEL_ERROR);
  OTTER_ASSERT(logger != NULL);

  proc_mgr = otter_process_manager_create(OTTER_TEST_ALLOCATOR, logger);
  OTTER_ASSERT(proc_mgr != NULL);

  /* A → B → C → A */
  static const otter_target_definition targets[] = {
      OBJECT_TARGET("target_a", target_b_deps),
      OBJECT_TARGET("target_b", target_c_deps),
      OBJECT_TARGET("target_c", target_a_deps), TARGET_LIST_END};

  otter_build_config config = {
      .paths = {.src_dir = "./test_src",
                .out_dir = "./test_out",
                .object_suffix = "",
                .shared_object_suffix = "",
                .executable_suffix = ""},
      .flags = {.cc_flags = "-Wall", .ll_flags = "", .include_flags = ""}};

  build_ctx = otter_build_context_create(targets, OTTER_TEST_ALLOCATOR,
                                         filesystem, logger, proc_mgr, &config);
  OTTER_ASSERT(build_ctx != NULL);

  /* Build should fail due to circular dependency */
  bool result = otter_build_all(build_ctx);
  OTTER_ASSERT(result == false);

  OTTER_TEST_END(if (build_ctx) otter_build_context_free(build_ctx);
                 if (proc_mgr) otter_process_manager_free(proc_mgr);
                 if (logger) otter_logger_free(logger);
                 if (filesystem) otter_filesystem_free(filesystem););
}

/* Test: Self-dependency (A → A) */
OTTER_TEST(build_detects_self_dependency) {
  otter_filesystem *filesystem = NULL;
  otter_logger *logger = NULL;
  otter_process_manager *proc_mgr = NULL;
  otter_build_context *build_ctx = NULL;

  filesystem = otter_filesystem_create(OTTER_TEST_ALLOCATOR);
  OTTER_ASSERT(filesystem != NULL);

  logger = otter_logger_create(OTTER_TEST_ALLOCATOR, OTTER_LOG_LEVEL_ERROR);
  OTTER_ASSERT(logger != NULL);

  proc_mgr = otter_process_manager_create(OTTER_TEST_ALLOCATOR, logger);
  OTTER_ASSERT(proc_mgr != NULL);

  /* Target depends on itself */
  static const otter_target_definition targets[] = {
      OBJECT_TARGET("self_dep", self_deps), TARGET_LIST_END};

  otter_build_config config = {
      .paths = {.src_dir = "./test_src",
                .out_dir = "./test_out",
                .object_suffix = "",
                .shared_object_suffix = "",
                .executable_suffix = ""},
      .flags = {.cc_flags = "-Wall", .ll_flags = "", .include_flags = ""}};

  build_ctx = otter_build_context_create(targets, OTTER_TEST_ALLOCATOR,
                                         filesystem, logger, proc_mgr, &config);
  OTTER_ASSERT(build_ctx != NULL);

  /* Build should fail due to self-dependency */
  bool result = otter_build_all(build_ctx);
  OTTER_ASSERT(result == false);

  OTTER_TEST_END(if (build_ctx) otter_build_context_free(build_ctx);
                 if (proc_mgr) otter_process_manager_free(proc_mgr);
                 if (logger) otter_logger_free(logger);
                 if (filesystem) otter_filesystem_free(filesystem););
}

/* Test: Duplicate target names */
OTTER_TEST(build_detects_duplicate_target_names) {
  otter_filesystem *filesystem = NULL;
  otter_logger *logger = NULL;
  otter_process_manager *proc_mgr = NULL;
  otter_build_context *build_ctx = NULL;

  filesystem = otter_filesystem_create(OTTER_TEST_ALLOCATOR);
  OTTER_ASSERT(filesystem != NULL);

  logger = otter_logger_create(OTTER_TEST_ALLOCATOR, OTTER_LOG_LEVEL_ERROR);
  OTTER_ASSERT(logger != NULL);

  proc_mgr = otter_process_manager_create(OTTER_TEST_ALLOCATOR, logger);
  OTTER_ASSERT(proc_mgr != NULL);

  /* Two targets with same name */
  static const otter_target_definition targets[] = {
      OBJECT_TARGET("duplicate", no_deps), OBJECT_TARGET("duplicate", no_deps),
      TARGET_LIST_END};

  otter_build_config config = {
      .paths = {.src_dir = "./test_src",
                .out_dir = "./test_out",
                .object_suffix = "",
                .shared_object_suffix = "",
                .executable_suffix = ""},
      .flags = {.cc_flags = "-Wall", .ll_flags = "", .include_flags = ""}};

  build_ctx = otter_build_context_create(targets, OTTER_TEST_ALLOCATOR,
                                         filesystem, logger, proc_mgr, &config);
  OTTER_ASSERT(build_ctx != NULL);

  /* Build should fail due to duplicate names */
  bool result = otter_build_all(build_ctx);
  OTTER_ASSERT(result == false);

  OTTER_TEST_END(if (build_ctx) otter_build_context_free(build_ctx);
                 if (proc_mgr) otter_process_manager_free(proc_mgr);
                 if (logger) otter_logger_free(logger);
                 if (filesystem) otter_filesystem_free(filesystem););
}

/* Test: Missing dependency */
OTTER_TEST(build_detects_missing_dependency) {
  otter_filesystem *filesystem = NULL;
  otter_logger *logger = NULL;
  otter_process_manager *proc_mgr = NULL;
  otter_build_context *build_ctx = NULL;

  filesystem = otter_filesystem_create(OTTER_TEST_ALLOCATOR);
  OTTER_ASSERT(filesystem != NULL);

  logger = otter_logger_create(OTTER_TEST_ALLOCATOR, OTTER_LOG_LEVEL_ERROR);
  OTTER_ASSERT(logger != NULL);

  proc_mgr = otter_process_manager_create(OTTER_TEST_ALLOCATOR, logger);
  OTTER_ASSERT(proc_mgr != NULL);

  /* Target depends on non-existent target */
  static const otter_target_definition targets[] = {
      OBJECT_TARGET("main", nonexistent_deps), TARGET_LIST_END};

  otter_build_config config = {
      .paths = {.src_dir = "./test_src",
                .out_dir = "./test_out",
                .object_suffix = "",
                .shared_object_suffix = "",
                .executable_suffix = ""},
      .flags = {.cc_flags = "-Wall", .ll_flags = "", .include_flags = ""}};

  build_ctx = otter_build_context_create(targets, OTTER_TEST_ALLOCATOR,
                                         filesystem, logger, proc_mgr, &config);
  OTTER_ASSERT(build_ctx != NULL);

  /* Build should fail due to missing dependency */
  bool result = otter_build_all(build_ctx);
  OTTER_ASSERT(result == false);

  OTTER_TEST_END(if (build_ctx) otter_build_context_free(build_ctx);
                 if (proc_mgr) otter_process_manager_free(proc_mgr);
                 if (logger) otter_logger_free(logger);
                 if (filesystem) otter_filesystem_free(filesystem););
}
