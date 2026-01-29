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
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

/* Test directory for integration tests */
#define TEST_DIR "/tmp/otter_build_test"
#define TEST_SRC_DIR TEST_DIR "/src"
#define TEST_OUT_DIR TEST_DIR "/out"

/* Dependency arrays */
static const char *no_deps[] = {NULL};

/* Constants for directory permissions and buffer sizes */
static const mode_t DIR_PERMISSIONS = 0755;
static const size_t PATH_BUFFER_SIZE = 512;

/* Helper to create test directory structure */
static bool setup_test_dirs(void) {
  /* Remove old test directory if it exists */
  system("rm -rf " TEST_DIR);

  /* Create directories */
  if (mkdir(TEST_DIR, DIR_PERMISSIONS) != 0) {
    return false;
  }
  if (mkdir(TEST_SRC_DIR, DIR_PERMISSIONS) != 0) {
    return false;
  }
  if (mkdir(TEST_OUT_DIR, DIR_PERMISSIONS) != 0) {
    return false;
  }
  return true;
}

/* Helper to create a simple C source file */
static bool create_source_file(const char *name, const char *content) {
  char path[PATH_BUFFER_SIZE];
  snprintf(path, sizeof(path), "%s/%s.c", TEST_SRC_DIR, name);

  FILE *file = fopen(path, "w");
  if (file == NULL) {
    return false;
  }

  fprintf(file, "%s", content);
  fclose(file);
  return true;
}

/* Helper to check if a file exists */
static bool file_exists(const char *path) {
  struct stat file_stat;
  return stat(path, &file_stat) == 0;
}

/* Test: Build a simple object file */
OTTER_TEST(build_integration_simple_object) {
  otter_filesystem *filesystem = NULL;
  otter_logger *logger = NULL;
  otter_process_manager *proc_mgr = NULL;
  otter_build_context *build_ctx = NULL;

  OTTER_ASSERT(setup_test_dirs());

  /* Create a simple C source file */
  const char *source = "int add(int a, int b) { return a + b; }\n";
  OTTER_ASSERT(create_source_file("math", source));

  filesystem = otter_filesystem_create(OTTER_TEST_ALLOCATOR);
  OTTER_ASSERT(filesystem != NULL);

  logger = otter_logger_create(OTTER_TEST_ALLOCATOR, OTTER_LOG_LEVEL_ERROR);
  OTTER_ASSERT(logger != NULL);

  proc_mgr = otter_process_manager_create(OTTER_TEST_ALLOCATOR, logger);
  OTTER_ASSERT(proc_mgr != NULL);

  static const otter_target_definition targets[] = {
      OBJECT_TARGET("math", no_deps), TARGET_LIST_END};

  otter_build_config config = {
      .paths = {.src_dir = TEST_SRC_DIR, .out_dir = TEST_OUT_DIR, .suffix = ""},
      .flags = {.cc_flags = "-Wall", .ll_flags = "", .include_flags = ""}};

  build_ctx = otter_build_context_create(targets, OTTER_TEST_ALLOCATOR,
                                         filesystem, logger, proc_mgr, &config);
  OTTER_ASSERT(build_ctx != NULL);

  /* Build the target */
  bool result = otter_build_all(build_ctx);
  OTTER_ASSERT(result == true);

  /* Verify output file exists */
  OTTER_ASSERT(file_exists(TEST_OUT_DIR "/math.o"));

  OTTER_TEST_END(if (build_ctx) otter_build_context_free(build_ctx);
                 if (proc_mgr) otter_process_manager_free(proc_mgr);
                 if (logger) otter_logger_free(logger);
                 if (filesystem) otter_filesystem_free(filesystem);
                 system("rm -rf " TEST_DIR););
}

/* Test: Build object file with dependency chain */
OTTER_TEST(build_integration_dependency_chain) {
  otter_filesystem *filesystem = NULL;
  otter_logger *logger = NULL;
  otter_process_manager *proc_mgr = NULL;
  otter_build_context *build_ctx = NULL;

  OTTER_ASSERT(setup_test_dirs());

  /* Create source files */
  OTTER_ASSERT(
      create_source_file("base", "int base_value(void) { return 42; }\n"));
  OTTER_ASSERT(create_source_file("derived",
                                  "int derived_value(void) { return 100; }\n"));
  OTTER_ASSERT(
      create_source_file("top", "int top_value(void) { return 200; }\n"));

  filesystem = otter_filesystem_create(OTTER_TEST_ALLOCATOR);
  OTTER_ASSERT(filesystem != NULL);

  logger = otter_logger_create(OTTER_TEST_ALLOCATOR, OTTER_LOG_LEVEL_ERROR);
  OTTER_ASSERT(logger != NULL);

  proc_mgr = otter_process_manager_create(OTTER_TEST_ALLOCATOR, logger);
  OTTER_ASSERT(proc_mgr != NULL);

  /* Create dependency chain: top -> derived -> base */
  static const char *derived_deps[] = {"base", NULL};
  static const char *top_deps[] = {"derived", NULL};
  static const otter_target_definition targets[] = {
      OBJECT_TARGET("base", no_deps), OBJECT_TARGET("derived", derived_deps),
      OBJECT_TARGET("top", top_deps), TARGET_LIST_END};

  otter_build_config config = {
      .paths = {.src_dir = TEST_SRC_DIR, .out_dir = TEST_OUT_DIR, .suffix = ""},
      .flags = {.cc_flags = "-Wall", .ll_flags = "", .include_flags = ""}};

  build_ctx = otter_build_context_create(targets, OTTER_TEST_ALLOCATOR,
                                         filesystem, logger, proc_mgr, &config);
  OTTER_ASSERT(build_ctx != NULL);

  /* Build all targets */
  bool result = otter_build_all(build_ctx);
  OTTER_ASSERT(result == true);

  /* Verify all output files exist */
  OTTER_ASSERT(file_exists(TEST_OUT_DIR "/base.o"));
  OTTER_ASSERT(file_exists(TEST_OUT_DIR "/derived.o"));
  OTTER_ASSERT(file_exists(TEST_OUT_DIR "/top.o"));

  OTTER_TEST_END(if (build_ctx) otter_build_context_free(build_ctx);
                 if (proc_mgr) otter_process_manager_free(proc_mgr);
                 if (logger) otter_logger_free(logger);
                 if (filesystem) otter_filesystem_free(filesystem);
                 system("rm -rf " TEST_DIR););
}

/* Test: Build executable from object files */
OTTER_TEST(build_integration_executable) {
  otter_filesystem *filesystem = NULL;
  otter_logger *logger = NULL;
  otter_process_manager *proc_mgr = NULL;
  otter_build_context *build_ctx = NULL;

  OTTER_ASSERT(setup_test_dirs());

  /* Create source files */
  OTTER_ASSERT(
      create_source_file("util", "int add(int a, int b) { return a + b; }\n"));
  OTTER_ASSERT(create_source_file("main",
                                  "int add(int, int);\n"
                                  "int main(void) { return add(2, 3); }\n"));

  filesystem = otter_filesystem_create(OTTER_TEST_ALLOCATOR);
  OTTER_ASSERT(filesystem != NULL);

  logger = otter_logger_create(OTTER_TEST_ALLOCATOR, OTTER_LOG_LEVEL_ERROR);
  OTTER_ASSERT(logger != NULL);

  proc_mgr = otter_process_manager_create(OTTER_TEST_ALLOCATOR, logger);
  OTTER_ASSERT(proc_mgr != NULL);

  /* Create executable that depends on util object */
  static const char *main_deps[] = {"util", NULL};
  static const otter_target_definition targets[] = {
      OBJECT_TARGET("util", no_deps), EXECUTABLE_TARGET("main", main_deps),
      TARGET_LIST_END};

  otter_build_config config = {
      .paths = {.src_dir = TEST_SRC_DIR, .out_dir = TEST_OUT_DIR, .suffix = ""},
      .flags = {.cc_flags = "-Wall", .ll_flags = "", .include_flags = ""}};

  build_ctx = otter_build_context_create(targets, OTTER_TEST_ALLOCATOR,
                                         filesystem, logger, proc_mgr, &config);
  OTTER_ASSERT(build_ctx != NULL);

  /* Build all targets */
  bool result = otter_build_all(build_ctx);
  OTTER_ASSERT(result == true);

  /* Verify output files exist */
  OTTER_ASSERT(file_exists(TEST_OUT_DIR "/util.o"));
  OTTER_ASSERT(file_exists(TEST_OUT_DIR "/main"));

  OTTER_TEST_END(if (build_ctx) otter_build_context_free(build_ctx);
                 if (proc_mgr) otter_process_manager_free(proc_mgr);
                 if (logger) otter_logger_free(logger);
                 if (filesystem) otter_filesystem_free(filesystem);
                 system("rm -rf " TEST_DIR););
}

/* Test: Build shared object */
OTTER_TEST(build_integration_shared_object) {
  otter_filesystem *filesystem = NULL;
  otter_logger *logger = NULL;
  otter_process_manager *proc_mgr = NULL;
  otter_build_context *build_ctx = NULL;

  OTTER_ASSERT(setup_test_dirs());

  /* Create source files for both object and shared library */
  OTTER_ASSERT(create_source_file(
      "plugin_impl", "int helper_function(void) { return 42; }\n"));
  OTTER_ASSERT(create_source_file(
      "plugin",
      "int helper_function(void);\n"
      "int plugin_function(void) { return helper_function() + 81; }\n"));

  filesystem = otter_filesystem_create(OTTER_TEST_ALLOCATOR);
  OTTER_ASSERT(filesystem != NULL);

  logger = otter_logger_create(OTTER_TEST_ALLOCATOR, OTTER_LOG_LEVEL_INFO);
  OTTER_ASSERT(logger != NULL);

  proc_mgr = otter_process_manager_create(OTTER_TEST_ALLOCATOR, logger);
  OTTER_ASSERT(proc_mgr != NULL);

  /* Build object first, then shared object that depends on it */
  static const char *plugin_deps[] = {"plugin_impl", NULL};
  static const otter_target_definition targets[] = {
      OBJECT_TARGET("plugin_impl", no_deps),
      SHARED_TARGET("plugin", plugin_deps, "-fPIC"), TARGET_LIST_END};

  otter_build_config config = {
      .paths = {.src_dir = TEST_SRC_DIR, .out_dir = TEST_OUT_DIR, .suffix = ""},
      .flags = {.cc_flags = "-Wall -fPIC",
                .ll_flags = "-shared",
                .include_flags = ""}};

  build_ctx = otter_build_context_create(targets, OTTER_TEST_ALLOCATOR,
                                         filesystem, logger, proc_mgr, &config);
  OTTER_ASSERT(build_ctx != NULL);

  /* Build the shared object */
  bool result = otter_build_all(build_ctx);
  OTTER_ASSERT(result == true);

  /* Verify output files exist */
  OTTER_ASSERT(file_exists(TEST_OUT_DIR "/plugin_impl.o"));
  OTTER_ASSERT(file_exists(TEST_OUT_DIR "/plugin.so"));

  OTTER_TEST_END(if (build_ctx) otter_build_context_free(build_ctx);
                 if (proc_mgr) otter_process_manager_free(proc_mgr);
                 if (logger) otter_logger_free(logger);
                 if (filesystem) otter_filesystem_free(filesystem);
                 system("rm -rf " TEST_DIR););
}

/* Test: Rebuild detection - no changes */
OTTER_TEST(build_integration_no_rebuild_needed) {
  otter_filesystem *filesystem = NULL;
  otter_logger *logger = NULL;
  otter_process_manager *proc_mgr = NULL;
  otter_build_context *build_ctx = NULL;

  OTTER_ASSERT(setup_test_dirs());

  /* Create a simple C source file */
  const char *source = "int value(void) { return 5; }\n";
  OTTER_ASSERT(create_source_file("constant", source));

  filesystem = otter_filesystem_create(OTTER_TEST_ALLOCATOR);
  OTTER_ASSERT(filesystem != NULL);

  logger = otter_logger_create(OTTER_TEST_ALLOCATOR, OTTER_LOG_LEVEL_INFO);
  OTTER_ASSERT(logger != NULL);

  proc_mgr = otter_process_manager_create(OTTER_TEST_ALLOCATOR, logger);
  OTTER_ASSERT(proc_mgr != NULL);

  static const otter_target_definition targets[] = {
      OBJECT_TARGET("constant", no_deps), TARGET_LIST_END};

  otter_build_config config = {
      .paths = {.src_dir = TEST_SRC_DIR, .out_dir = TEST_OUT_DIR, .suffix = ""},
      .flags = {.cc_flags = "-Wall", .ll_flags = "", .include_flags = ""}};

  build_ctx = otter_build_context_create(targets, OTTER_TEST_ALLOCATOR,
                                         filesystem, logger, proc_mgr, &config);
  OTTER_ASSERT(build_ctx != NULL);

  /* First build */
  bool result = otter_build_all(build_ctx);
  OTTER_ASSERT(result == true);
  OTTER_ASSERT(file_exists(TEST_OUT_DIR "/constant.o"));

  /* Second build - should detect no changes needed */
  result = otter_build_all(build_ctx);
  OTTER_ASSERT(result == true);

  OTTER_TEST_END(if (build_ctx) otter_build_context_free(build_ctx);
                 if (proc_mgr) otter_process_manager_free(proc_mgr);
                 if (logger) otter_logger_free(logger);
                 if (filesystem) otter_filesystem_free(filesystem);
                 system("rm -rf " TEST_DIR););
}

/* Test: Rebuild detection - source modified (hash-based detection) */
OTTER_TEST(build_integration_rebuild_after_modification) {
  otter_filesystem *filesystem = NULL;
  otter_logger *logger = NULL;
  otter_process_manager *proc_mgr = NULL;
  otter_build_context *build_ctx = NULL;

  OTTER_ASSERT(setup_test_dirs());

  /* Create initial source file */
  const char *source1 = "int value(void) { return 5; }\n";
  OTTER_ASSERT(create_source_file("changing", source1));

  filesystem = otter_filesystem_create(OTTER_TEST_ALLOCATOR);
  OTTER_ASSERT(filesystem != NULL);

  logger = otter_logger_create(OTTER_TEST_ALLOCATOR, OTTER_LOG_LEVEL_ERROR);
  OTTER_ASSERT(logger != NULL);

  proc_mgr = otter_process_manager_create(OTTER_TEST_ALLOCATOR, logger);
  OTTER_ASSERT(proc_mgr != NULL);

  static const otter_target_definition targets[] = {
      OBJECT_TARGET("changing", no_deps), TARGET_LIST_END};

  otter_build_config config = {
      .paths = {.src_dir = TEST_SRC_DIR, .out_dir = TEST_OUT_DIR, .suffix = ""},
      .flags = {.cc_flags = "-Wall", .ll_flags = "", .include_flags = ""}};

  build_ctx = otter_build_context_create(targets, OTTER_TEST_ALLOCATOR,
                                         filesystem, logger, proc_mgr, &config);
  OTTER_ASSERT(build_ctx != NULL);

  /* First build */
  bool result = otter_build_all(build_ctx);
  OTTER_ASSERT(result == true);
  OTTER_ASSERT(file_exists(TEST_OUT_DIR "/changing.o"));

  /* Modify source file (content changes, so hash will change) */
  const char *source2 = "int value(void) { return 10; }\n";
  OTTER_ASSERT(create_source_file("changing", source2));

  /* Rebuild - should detect change via hash comparison and rebuild */
  result = otter_build_all(build_ctx);
  OTTER_ASSERT(result == true);

  /* Verify output still exists (hash-based system rebuilds when content
   * changes) */
  OTTER_ASSERT(file_exists(TEST_OUT_DIR "/changing.o"));

  OTTER_TEST_END(if (build_ctx) otter_build_context_free(build_ctx);
                 if (proc_mgr) otter_process_manager_free(proc_mgr);
                 if (logger) otter_logger_free(logger);
                 if (filesystem) otter_filesystem_free(filesystem);
                 system("rm -rf " TEST_DIR););
}

/* Test: Build with suffix */
OTTER_TEST(build_integration_with_suffix) {
  otter_filesystem *filesystem = NULL;
  otter_logger *logger = NULL;
  otter_process_manager *proc_mgr = NULL;
  otter_build_context *build_ctx = NULL;

  OTTER_ASSERT(setup_test_dirs());

  /* Create source file */
  OTTER_ASSERT(
      create_source_file("test", "int test_func(void) { return 1; }\n"));

  filesystem = otter_filesystem_create(OTTER_TEST_ALLOCATOR);
  OTTER_ASSERT(filesystem != NULL);

  logger = otter_logger_create(OTTER_TEST_ALLOCATOR, OTTER_LOG_LEVEL_ERROR);
  OTTER_ASSERT(logger != NULL);

  proc_mgr = otter_process_manager_create(OTTER_TEST_ALLOCATOR, logger);
  OTTER_ASSERT(proc_mgr != NULL);

  static const otter_target_definition targets[] = {
      OBJECT_TARGET("test", no_deps), TARGET_LIST_END};

  otter_build_config config = {
      .paths = {.src_dir = TEST_SRC_DIR,
                .out_dir = TEST_OUT_DIR,
                .suffix = "_debug"},
      .flags = {.cc_flags = "-g", .ll_flags = "", .include_flags = ""}};

  build_ctx = otter_build_context_create(targets, OTTER_TEST_ALLOCATOR,
                                         filesystem, logger, proc_mgr, &config);
  OTTER_ASSERT(build_ctx != NULL);

  /* Build the target */
  bool result = otter_build_all(build_ctx);
  OTTER_ASSERT(result == true);

  /* Verify output file has suffix */
  OTTER_ASSERT(file_exists(TEST_OUT_DIR "/test_debug.o"));

  OTTER_TEST_END(if (build_ctx) otter_build_context_free(build_ctx);
                 if (proc_mgr) otter_process_manager_free(proc_mgr);
                 if (logger) otter_logger_free(logger);
                 if (filesystem) otter_filesystem_free(filesystem);
                 system("rm -rf " TEST_DIR););
}

/* Test: Build with custom source file name */
OTTER_TEST(build_integration_custom_source_name) {
  otter_filesystem *filesystem = NULL;
  otter_logger *logger = NULL;
  otter_process_manager *proc_mgr = NULL;
  otter_build_context *build_ctx = NULL;

  OTTER_ASSERT(setup_test_dirs());

  /* Create source file with different name than target */
  OTTER_ASSERT(create_source_file("impl", "int func(void) { return 99; }\n"));

  filesystem = otter_filesystem_create(OTTER_TEST_ALLOCATOR);
  OTTER_ASSERT(filesystem != NULL);

  logger = otter_logger_create(OTTER_TEST_ALLOCATOR, OTTER_LOG_LEVEL_ERROR);
  OTTER_ASSERT(logger != NULL);

  proc_mgr = otter_process_manager_create(OTTER_TEST_ALLOCATOR, logger);
  OTTER_ASSERT(proc_mgr != NULL);

  /* Target name is "interface" but source is "impl" */
  static const otter_target_definition targets[] = {
      OBJECT_TARGET_SRC("interface", "impl", no_deps), TARGET_LIST_END};

  otter_build_config config = {
      .paths = {.src_dir = TEST_SRC_DIR, .out_dir = TEST_OUT_DIR, .suffix = ""},
      .flags = {.cc_flags = "-Wall", .ll_flags = "", .include_flags = ""}};

  build_ctx = otter_build_context_create(targets, OTTER_TEST_ALLOCATOR,
                                         filesystem, logger, proc_mgr, &config);
  OTTER_ASSERT(build_ctx != NULL);

  /* Build the target */
  bool result = otter_build_all(build_ctx);
  OTTER_ASSERT(result == true);

  /* Verify output uses target name, not source name */
  OTTER_ASSERT(file_exists(TEST_OUT_DIR "/interface.o"));

  OTTER_TEST_END(if (build_ctx) otter_build_context_free(build_ctx);
                 if (proc_mgr) otter_process_manager_free(proc_mgr);
                 if (logger) otter_logger_free(logger);
                 if (filesystem) otter_filesystem_free(filesystem);
                 system("rm -rf " TEST_DIR););
}

/* Test: Build failure - missing source file */
OTTER_TEST(build_integration_missing_source) {
  otter_filesystem *filesystem = NULL;
  otter_logger *logger = NULL;
  otter_process_manager *proc_mgr = NULL;
  otter_build_context *build_ctx = NULL;

  OTTER_ASSERT(setup_test_dirs());

  /* Don't create source file - intentionally missing */

  filesystem = otter_filesystem_create(OTTER_TEST_ALLOCATOR);
  OTTER_ASSERT(filesystem != NULL);

  logger = otter_logger_create(OTTER_TEST_ALLOCATOR, OTTER_LOG_LEVEL_ERROR);
  OTTER_ASSERT(logger != NULL);

  proc_mgr = otter_process_manager_create(OTTER_TEST_ALLOCATOR, logger);
  OTTER_ASSERT(proc_mgr != NULL);

  static const otter_target_definition targets[] = {
      OBJECT_TARGET("nonexistent", no_deps), TARGET_LIST_END};

  otter_build_config config = {
      .paths = {.src_dir = TEST_SRC_DIR, .out_dir = TEST_OUT_DIR, .suffix = ""},
      .flags = {.cc_flags = "-Wall", .ll_flags = "", .include_flags = ""}};

  build_ctx = otter_build_context_create(targets, OTTER_TEST_ALLOCATOR,
                                         filesystem, logger, proc_mgr, &config);
  OTTER_ASSERT(build_ctx != NULL);

  /* Build should fail - source doesn't exist */
  bool result = otter_build_all(build_ctx);
  OTTER_ASSERT(result == false);

  OTTER_TEST_END(if (build_ctx) otter_build_context_free(build_ctx);
                 if (proc_mgr) otter_process_manager_free(proc_mgr);
                 if (logger) otter_logger_free(logger);
                 if (filesystem) otter_filesystem_free(filesystem);
                 system("rm -rf " TEST_DIR););
}

/* Test: Build failure - compilation error */
OTTER_TEST(build_integration_compilation_error) {
  otter_filesystem *filesystem = NULL;
  otter_logger *logger = NULL;
  otter_process_manager *proc_mgr = NULL;
  otter_build_context *build_ctx = NULL;

  OTTER_ASSERT(setup_test_dirs());

  /* Create source file with syntax error */
  const char *bad_source =
      "int broken(void) { return; }\n"; /* Missing return value */
  OTTER_ASSERT(create_source_file("broken", bad_source));

  filesystem = otter_filesystem_create(OTTER_TEST_ALLOCATOR);
  OTTER_ASSERT(filesystem != NULL);

  logger = otter_logger_create(OTTER_TEST_ALLOCATOR, OTTER_LOG_LEVEL_ERROR);
  OTTER_ASSERT(logger != NULL);

  proc_mgr = otter_process_manager_create(OTTER_TEST_ALLOCATOR, logger);
  OTTER_ASSERT(proc_mgr != NULL);

  static const otter_target_definition targets[] = {
      OBJECT_TARGET("broken", no_deps), TARGET_LIST_END};

  otter_build_config config = {
      .paths = {.src_dir = TEST_SRC_DIR, .out_dir = TEST_OUT_DIR, .suffix = ""},
      .flags = {
          .cc_flags = "-Wall -Werror", .ll_flags = "", .include_flags = ""}};

  build_ctx = otter_build_context_create(targets, OTTER_TEST_ALLOCATOR,
                                         filesystem, logger, proc_mgr, &config);
  OTTER_ASSERT(build_ctx != NULL);

  /* Build should fail due to compilation error */
  bool result = otter_build_all(build_ctx);
  OTTER_ASSERT(result == false);

  OTTER_TEST_END(if (build_ctx) otter_build_context_free(build_ctx);
                 if (proc_mgr) otter_process_manager_free(proc_mgr);
                 if (logger) otter_logger_free(logger);
                 if (filesystem) otter_filesystem_free(filesystem);
                 system("rm -rf " TEST_DIR););
}

/* Test: Multiple independent targets build in parallel */
OTTER_TEST(build_integration_parallel_builds) {
  otter_filesystem *filesystem = NULL;
  otter_logger *logger = NULL;
  otter_process_manager *proc_mgr = NULL;
  otter_build_context *build_ctx = NULL;

  OTTER_ASSERT(setup_test_dirs());

  /* Create multiple independent source files */
  OTTER_ASSERT(create_source_file("mod_a", "int a(void) { return 1; }\n"));
  OTTER_ASSERT(create_source_file("mod_b", "int b(void) { return 2; }\n"));
  OTTER_ASSERT(create_source_file("mod_c", "int c(void) { return 3; }\n"));

  filesystem = otter_filesystem_create(OTTER_TEST_ALLOCATOR);
  OTTER_ASSERT(filesystem != NULL);

  logger = otter_logger_create(OTTER_TEST_ALLOCATOR, OTTER_LOG_LEVEL_ERROR);
  OTTER_ASSERT(logger != NULL);

  proc_mgr = otter_process_manager_create(OTTER_TEST_ALLOCATOR, logger);
  OTTER_ASSERT(proc_mgr != NULL);

  static const otter_target_definition targets[] = {
      OBJECT_TARGET("mod_a", no_deps), OBJECT_TARGET("mod_b", no_deps),
      OBJECT_TARGET("mod_c", no_deps), TARGET_LIST_END};

  otter_build_config config = {
      .paths = {.src_dir = TEST_SRC_DIR, .out_dir = TEST_OUT_DIR, .suffix = ""},
      .flags = {.cc_flags = "-Wall", .ll_flags = "", .include_flags = ""}};

  build_ctx = otter_build_context_create(targets, OTTER_TEST_ALLOCATOR,
                                         filesystem, logger, proc_mgr, &config);
  OTTER_ASSERT(build_ctx != NULL);

  /* Build all targets */
  bool result = otter_build_all(build_ctx);
  OTTER_ASSERT(result == true);

  /* Verify all outputs exist */
  OTTER_ASSERT(file_exists(TEST_OUT_DIR "/mod_a.o"));
  OTTER_ASSERT(file_exists(TEST_OUT_DIR "/mod_b.o"));
  OTTER_ASSERT(file_exists(TEST_OUT_DIR "/mod_c.o"));

  OTTER_TEST_END(if (build_ctx) otter_build_context_free(build_ctx);
                 if (proc_mgr) otter_process_manager_free(proc_mgr);
                 if (logger) otter_logger_free(logger);
                 if (filesystem) otter_filesystem_free(filesystem);
                 system("rm -rf " TEST_DIR););
}
