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

/* Dependency arrays */
static const char *no_deps[] = {NULL};

/* Constants for allocation failure tests */
static const size_t ALLOC_FAIL_AFTER_FLAGS = 3;
static const size_t ALLOC_FAIL_AFTER_CONTEXT = 5;
static const size_t ALLOC_FAIL_AFTER_FIRST_TARGET = 8;

/* Mock allocator that always returns NULL */
static void *null_malloc(otter_allocator * /* unused */, size_t /* unused */) {
  return NULL;
}

static void *null_realloc(otter_allocator * /* unused */, void * /* unused */,
                          size_t /* unused */) {
  return NULL;
}

static void null_free(otter_allocator * /* unused */, void * /* unused */) {
  /* No-op */
}

/* Counter-based allocator that fails after N allocations */
typedef struct {
  otter_allocator base;
  otter_allocator *real_allocator;
  size_t fail_after;
  size_t call_count;
} counting_allocator;

static void *counting_malloc(otter_allocator *alloc, size_t size) {
  counting_allocator *counting_alloc = (counting_allocator *)alloc;
  counting_alloc->call_count++;
  if (counting_alloc->call_count > counting_alloc->fail_after) {
    return NULL;
  }
  return otter_malloc(counting_alloc->real_allocator, size);
}

static void *counting_realloc(otter_allocator *alloc, void *ptr, size_t size) {
  counting_allocator *counting_alloc = (counting_allocator *)alloc;
  counting_alloc->call_count++;
  if (counting_alloc->call_count > counting_alloc->fail_after) {
    return NULL;
  }
  return otter_realloc(counting_alloc->real_allocator, ptr, size);
}

static void counting_free(otter_allocator *alloc, void *ptr) {
  counting_allocator *counting_alloc = (counting_allocator *)alloc;
  otter_free(counting_alloc->real_allocator, ptr);
}

/* Test: NULL context to otter_build_all */
OTTER_TEST(build_extended_null_context) {
  bool result = otter_build_all(NULL);
  OTTER_ASSERT(result == false);

  OTTER_TEST_END();
}

/* Test: NULL parameters to otter_build_context_create */
OTTER_TEST(build_extended_context_create_null_params) {
  otter_filesystem *filesystem = NULL;
  otter_logger *logger = NULL;
  otter_process_manager *proc_mgr = NULL;

  filesystem = otter_filesystem_create(OTTER_TEST_ALLOCATOR);
  OTTER_ASSERT(filesystem != NULL);

  logger = otter_logger_create(OTTER_TEST_ALLOCATOR, OTTER_LOG_LEVEL_ERROR);
  OTTER_ASSERT(logger != NULL);

  proc_mgr = otter_process_manager_create(OTTER_TEST_ALLOCATOR, logger);
  OTTER_ASSERT(proc_mgr != NULL);

  static const otter_target_definition targets[] = {
      OBJECT_TARGET("test", no_deps), TARGET_LIST_END};

  otter_build_config config = {
      .paths = {.src_dir = "./test_src",
                .out_dir = "./test_out",
                .object_suffix = "",
                .shared_object_suffix = "",
                .executable_suffix = ""},
      .flags = {.cc_flags = "-Wall", .ll_flags = "", .include_flags = ""}};

  /* NULL targets */
  otter_build_context *ctx1 = otter_build_context_create(
      NULL, OTTER_TEST_ALLOCATOR, filesystem, logger, proc_mgr, &config);
  OTTER_ASSERT(ctx1 == NULL);

  /* NULL allocator */
  otter_build_context *ctx2 = otter_build_context_create(
      targets, NULL, filesystem, logger, proc_mgr, &config);
  OTTER_ASSERT(ctx2 == NULL);

  /* NULL filesystem */
  otter_build_context *ctx3 = otter_build_context_create(
      targets, OTTER_TEST_ALLOCATOR, NULL, logger, proc_mgr, &config);
  OTTER_ASSERT(ctx3 == NULL);

  /* NULL logger */
  otter_build_context *ctx4 = otter_build_context_create(
      targets, OTTER_TEST_ALLOCATOR, filesystem, NULL, proc_mgr, &config);
  OTTER_ASSERT(ctx4 == NULL);

  /* NULL process_manager */
  otter_build_context *ctx5 = otter_build_context_create(
      targets, OTTER_TEST_ALLOCATOR, filesystem, logger, NULL, &config);
  OTTER_ASSERT(ctx5 == NULL);

  /* NULL config */
  otter_build_context *ctx6 = otter_build_context_create(
      targets, OTTER_TEST_ALLOCATOR, filesystem, logger, proc_mgr, NULL);
  OTTER_ASSERT(ctx6 == NULL);

  OTTER_TEST_END(if (proc_mgr) otter_process_manager_free(proc_mgr);
                 if (logger) otter_logger_free(logger);
                 if (filesystem) otter_filesystem_free(filesystem););
}

/* Test: Executable target with dependencies */
OTTER_TEST(build_extended_executable_with_deps) {
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

  static const char *main_deps[] = {"lib", NULL};
  static const otter_target_definition targets[] = {
      OBJECT_TARGET("lib", no_deps), EXECUTABLE_TARGET("main", main_deps),
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

  OTTER_TEST_END(if (build_ctx) otter_build_context_free(build_ctx);
                 if (proc_mgr) otter_process_manager_free(proc_mgr);
                 if (logger) otter_logger_free(logger);
                 if (filesystem) otter_filesystem_free(filesystem););
}

/* Test: Shared object target with dependencies */
OTTER_TEST(build_extended_shared_object_with_deps) {
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

  static const char *plugin_deps[] = {"util", NULL};
  static const otter_target_definition targets[] = {
      OBJECT_TARGET("util", no_deps),
      SHARED_TARGET("plugin", plugin_deps, "-fPIC"), TARGET_LIST_END};

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

/* Test: Complex dependency chain (A -> B -> C) */
OTTER_TEST(build_extended_complex_dependency_chain) {
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

  static const char *b_deps[] = {"c", NULL};
  static const char *a_deps[] = {"b", NULL};
  static const otter_target_definition targets[] = {
      OBJECT_TARGET("c", no_deps), OBJECT_TARGET("b", b_deps),
      OBJECT_TARGET("a", a_deps), TARGET_LIST_END};

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

  /* Don't actually execute - just verify context creation succeeded */

  OTTER_TEST_END(if (build_ctx) otter_build_context_free(build_ctx);
                 if (proc_mgr) otter_process_manager_free(proc_mgr);
                 if (logger) otter_logger_free(logger);
                 if (filesystem) otter_filesystem_free(filesystem););
}

/* Test: Multiple independent targets */
OTTER_TEST(build_extended_multiple_independent_targets) {
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
      OBJECT_TARGET("module1", no_deps), OBJECT_TARGET("module2", no_deps),
      OBJECT_TARGET("module3", no_deps), TARGET_LIST_END};

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

  /* Don't actually execute - just verify context creation succeeded */

  OTTER_TEST_END(if (build_ctx) otter_build_context_free(build_ctx);
                 if (proc_mgr) otter_process_manager_free(proc_mgr);
                 if (logger) otter_logger_free(logger);
                 if (filesystem) otter_filesystem_free(filesystem););
}

/* Test: Empty target list */
OTTER_TEST(build_extended_empty_target_list) {
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

  static const otter_target_definition targets[] = {TARGET_LIST_END};

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

  /* Should succeed - empty list is valid */
  bool result = otter_build_all(build_ctx);
  OTTER_ASSERT(result == true);

  OTTER_TEST_END(if (build_ctx) otter_build_context_free(build_ctx);
                 if (proc_mgr) otter_process_manager_free(proc_mgr);
                 if (logger) otter_logger_free(logger);
                 if (filesystem) otter_filesystem_free(filesystem););
}

/* Test: Target with custom source file name */
OTTER_TEST(build_extended_target_custom_source) {
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
      OBJECT_TARGET_SRC("output_name", "different_source", no_deps),
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

  OTTER_TEST_END(if (build_ctx) otter_build_context_free(build_ctx);
                 if (proc_mgr) otter_process_manager_free(proc_mgr);
                 if (logger) otter_logger_free(logger);
                 if (filesystem) otter_filesystem_free(filesystem););
}

/* Test: Target with suffix */
OTTER_TEST(build_extended_target_with_suffix) {
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
      OBJECT_TARGET("test", no_deps), TARGET_LIST_END};

  otter_build_config config = {.paths =
                                   {
                                       .src_dir = "./test_src",
                                       .out_dir = "./test_out",
                                       .object_suffix = "_debug",
                                       .shared_object_suffix = "_debug",
                                       .executable_suffix = "_debug",
                                   },
                               .flags = {.cc_flags = "-g",
                                         .ll_flags = "-g",
                                         .include_flags = "-I./include"}};

  build_ctx = otter_build_context_create(targets, OTTER_TEST_ALLOCATOR,
                                         filesystem, logger, proc_mgr, &config);
  OTTER_ASSERT(build_ctx != NULL);

  OTTER_TEST_END(if (build_ctx) otter_build_context_free(build_ctx);
                 if (proc_mgr) otter_process_manager_free(proc_mgr);
                 if (logger) otter_logger_free(logger);
                 if (filesystem) otter_filesystem_free(filesystem););
}

/* Test: Executable target with source file */
OTTER_TEST(build_extended_executable_with_source) {
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
      EXECUTABLE_TARGET_SRC("myapp", "main", no_deps), TARGET_LIST_END};

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

/* Test: Allocation failure during context creation (flags string) */
OTTER_TEST(build_extended_alloc_failure_flags_string) {
  otter_filesystem *filesystem = NULL;
  otter_logger *logger = NULL;
  otter_process_manager *proc_mgr = NULL;

  filesystem = otter_filesystem_create(OTTER_TEST_ALLOCATOR);
  OTTER_ASSERT(filesystem != NULL);

  logger = otter_logger_create(OTTER_TEST_ALLOCATOR, OTTER_LOG_LEVEL_ERROR);
  OTTER_ASSERT(logger != NULL);

  proc_mgr = otter_process_manager_create(OTTER_TEST_ALLOCATOR, logger);
  OTTER_ASSERT(proc_mgr != NULL);

  static const otter_target_definition targets[] = {
      OBJECT_TARGET("test", no_deps), TARGET_LIST_END};

  otter_build_config config = {
      .paths = {.src_dir = "./test_src",
                .out_dir = "./test_out",
                .object_suffix = "",
                .shared_object_suffix = "",
                .executable_suffix = ""},
      .flags = {.cc_flags = "-Wall", .ll_flags = "", .include_flags = ""}};

  /* Use null allocator - should fail to create context */
  otter_allocator_vtable vtable = {
      .malloc = null_malloc,
      .realloc = null_realloc,
      .free = null_free,
  };

  otter_allocator null_alloc = {
      .vtable = &vtable,
  };

  otter_build_context *ctx = otter_build_context_create(
      targets, &null_alloc, filesystem, logger, proc_mgr, &config);
  OTTER_ASSERT(ctx == NULL);

  OTTER_TEST_END(if (proc_mgr) otter_process_manager_free(proc_mgr);
                 if (logger) otter_logger_free(logger);
                 if (filesystem) otter_filesystem_free(filesystem););
}

/* Test: Allocation failure during context struct allocation */
OTTER_TEST(build_extended_alloc_failure_context_struct) {
  otter_filesystem *filesystem = NULL;
  otter_logger *logger = NULL;
  otter_process_manager *proc_mgr = NULL;

  filesystem = otter_filesystem_create(OTTER_TEST_ALLOCATOR);
  OTTER_ASSERT(filesystem != NULL);

  logger = otter_logger_create(OTTER_TEST_ALLOCATOR, OTTER_LOG_LEVEL_ERROR);
  OTTER_ASSERT(logger != NULL);

  proc_mgr = otter_process_manager_create(OTTER_TEST_ALLOCATOR, logger);
  OTTER_ASSERT(proc_mgr != NULL);

  static const otter_target_definition targets[] = {
      OBJECT_TARGET("test", no_deps), TARGET_LIST_END};

  otter_build_config config = {
      .paths = {.src_dir = "./test_src",
                .out_dir = "./test_out",
                .object_suffix = "",
                .shared_object_suffix = "",
                .executable_suffix = ""},
      .flags = {.cc_flags = "-Wall", .ll_flags = "", .include_flags = ""}};

  /* Allocator that fails after first allocation (flags strings succeed, context
   * struct fails) */
  otter_allocator_vtable counting_vtable = {
      .malloc = counting_malloc,
      .realloc = counting_realloc,
      .free = counting_free,
  };

  counting_allocator counting_alloc = {
      .base = {.vtable = &counting_vtable},
      .real_allocator = OTTER_TEST_ALLOCATOR,
      .fail_after = ALLOC_FAIL_AFTER_FLAGS, /* Allow flags strings, fail on
                                               context or targets array */
      .call_count = 0,
  };

  otter_build_context *ctx =
      otter_build_context_create(targets, (otter_allocator *)&counting_alloc,
                                 filesystem, logger, proc_mgr, &config);
  OTTER_ASSERT(ctx == NULL);

  OTTER_TEST_END(if (proc_mgr) otter_process_manager_free(proc_mgr);
                 if (logger) otter_logger_free(logger);
                 if (filesystem) otter_filesystem_free(filesystem););
}

/* Test: String format allocation failure in create_path */
OTTER_TEST(build_extended_alloc_failure_create_path) {
  otter_filesystem *filesystem = NULL;
  otter_logger *logger = NULL;
  otter_process_manager *proc_mgr = NULL;

  filesystem = otter_filesystem_create(OTTER_TEST_ALLOCATOR);
  OTTER_ASSERT(filesystem != NULL);

  logger = otter_logger_create(OTTER_TEST_ALLOCATOR, OTTER_LOG_LEVEL_ERROR);
  OTTER_ASSERT(logger != NULL);

  proc_mgr = otter_process_manager_create(OTTER_TEST_ALLOCATOR, logger);
  OTTER_ASSERT(proc_mgr != NULL);

  static const otter_target_definition targets[] = {
      OBJECT_TARGET("test", no_deps), TARGET_LIST_END};

  otter_build_config config = {
      .paths = {.src_dir = "./test_src",
                .out_dir = "./test_out",
                .object_suffix = "",
                .shared_object_suffix = "",
                .executable_suffix = ""},
      .flags = {.cc_flags = "-Wall", .ll_flags = "", .include_flags = ""}};

  /* Allocator that fails after context creation succeeds */
  otter_allocator_vtable counting_vtable = {
      .malloc = counting_malloc,
      .realloc = counting_realloc,
      .free = counting_free,
  };

  counting_allocator counting_alloc = {
      .base = {.vtable = &counting_vtable},
      .real_allocator = OTTER_TEST_ALLOCATOR,
      .fail_after = ALLOC_FAIL_AFTER_CONTEXT, /* Allow context creation, fail
                                                 during target creation */
      .call_count = 0,
  };

  otter_build_context *ctx =
      otter_build_context_create(targets, (otter_allocator *)&counting_alloc,
                                 filesystem, logger, proc_mgr, &config);

  if (ctx != NULL) {
    /* Context created, now try to build - should fail during target creation */
    bool result = otter_build_all(ctx);
    OTTER_ASSERT(result == false);
    otter_build_context_free(ctx);
  }

  OTTER_TEST_END(if (proc_mgr) otter_process_manager_free(proc_mgr);
                 if (logger) otter_logger_free(logger);
                 if (filesystem) otter_filesystem_free(filesystem););
}

/* Test: Dependency array allocation failure */
OTTER_TEST(build_extended_alloc_failure_dependency_array) {
  otter_filesystem *filesystem = NULL;
  otter_logger *logger = NULL;
  otter_process_manager *proc_mgr = NULL;

  filesystem = otter_filesystem_create(OTTER_TEST_ALLOCATOR);
  OTTER_ASSERT(filesystem != NULL);

  logger = otter_logger_create(OTTER_TEST_ALLOCATOR, OTTER_LOG_LEVEL_ERROR);
  OTTER_ASSERT(logger != NULL);

  proc_mgr = otter_process_manager_create(OTTER_TEST_ALLOCATOR, logger);
  OTTER_ASSERT(proc_mgr != NULL);

  static const char *main_deps[] = {"lib", NULL};
  static const otter_target_definition targets[] = {
      OBJECT_TARGET("lib", no_deps), EXECUTABLE_TARGET("main", main_deps),
      TARGET_LIST_END};

  otter_build_config config = {
      .paths = {.src_dir = "./test_src",
                .out_dir = "./test_out",
                .object_suffix = "",
                .shared_object_suffix = "",
                .executable_suffix = ""},
      .flags = {.cc_flags = "-Wall", .ll_flags = "", .include_flags = ""}};

  /* Allocator that fails after allowing context and first target */
  otter_allocator_vtable counting_vtable = {
      .malloc = counting_malloc,
      .realloc = counting_realloc,
      .free = counting_free,
  };

  counting_allocator counting_alloc = {
      .base = {.vtable = &counting_vtable},
      .real_allocator = OTTER_TEST_ALLOCATOR,
      .fail_after =
          ALLOC_FAIL_AFTER_FIRST_TARGET, /* Fail when creating dependency array
                                            for executable */
      .call_count = 0,
  };

  otter_build_context *ctx =
      otter_build_context_create(targets, (otter_allocator *)&counting_alloc,
                                 filesystem, logger, proc_mgr, &config);

  if (ctx != NULL) {
    bool result = otter_build_all(ctx);
    OTTER_ASSERT(result == false);
    otter_build_context_free(ctx);
  }

  OTTER_TEST_END(if (proc_mgr) otter_process_manager_free(proc_mgr);
                 if (logger) otter_logger_free(logger);
                 if (filesystem) otter_filesystem_free(filesystem););
}

/* Test: Target with extra flags requiring string concatenation */
OTTER_TEST(build_extended_target_with_extra_flags) {
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
      SHARED_TARGET("plugin", no_deps, "-fPIC -shared"), TARGET_LIST_END};

  otter_build_config config = {.paths = {.src_dir = "./test_src",
                                         .out_dir = "./test_out",
                                         .object_suffix = "",
                                         .shared_object_suffix = "",
                                         .executable_suffix = ""},
                               .flags = {.cc_flags = "-Wall",
                                         .ll_flags = "-Wl,-z,relro",
                                         .include_flags = "-I."}};

  build_ctx = otter_build_context_create(targets, OTTER_TEST_ALLOCATOR,
                                         filesystem, logger, proc_mgr, &config);
  OTTER_ASSERT(build_ctx != NULL);

  OTTER_TEST_END(if (build_ctx) otter_build_context_free(build_ctx);
                 if (proc_mgr) otter_process_manager_free(proc_mgr);
                 if (logger) otter_logger_free(logger);
                 if (filesystem) otter_filesystem_free(filesystem););
}
