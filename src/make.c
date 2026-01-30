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
#include "otter/build.h"
#include "otter/filesystem.h"
#include "otter/inc.h"
#include "otter/logger.h"
#include "otter/process_manager.h"

#include <stdbool.h>
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

/* Main build target dependencies */
static const char *allocator_deps[] = {NULL};
static const char *string_deps[] = {"allocator", NULL};
static const char *array_deps[] = {"allocator", NULL};
static const char *cstring_deps[] = {"allocator", NULL};
static const char *logger_deps[] = {"cstring", "array", "allocator", NULL};
static const char *process_manager_deps[] = {"allocator", "logger", "string",
                                             NULL};
static const char *file_deps[] = {NULL};
static const char *filesystem_deps[] = {"file", "allocator", NULL};
static const char *target_deps[] = {"allocator", "array",  "filesystem",
                                    "logger",    "string", NULL};
static const char *token_deps[] = {"allocator", NULL};
static const char *node_deps[] = {"allocator", "array", NULL};
static const char *lexer_deps[] = {"array", "cstring", NULL};
static const char *parser_deps[] = {"allocator", "logger", "node",
                                    "cstring",   "token",  NULL};
static const char *bytecode_deps[] = {NULL};
static const char *vm_deps[] = {"allocator", "logger", "bytecode", NULL};
static const char *test_deps[] = {"allocator", NULL};
static const char *build_deps[] = {
    "allocator", "filesystem", "logger", "process_manager",
    "target",    "string",     NULL};
static const char *cstring_tests_deps[] = {"test", "cstring", NULL};
static const char *string_tests_deps[] = {"test", "string", NULL};
static const char *array_tests_deps[] = {"test", "array", NULL};
static const char *lexer_tests_deps[] = {"test", "lexer", "token", NULL};
static const char *parser_tests_deps[] = {"test", "cstring", "node", "parser",
                                          NULL};
static const char *parser_integration_tests_deps[] = {"test", "lexer", "node",
                                                      "parser", NULL};
static const char *build_tests_deps[] = {"test", "build", "filesystem",
                                         "logger", NULL};
static const char *build_tests_extended_deps[] = {"test", "build", "filesystem",
                                                  "logger", NULL};
static const char *build_integration_tests_deps[] = {
    "test", "build", "filesystem", "logger", NULL};
static const char *otter_exe_deps[] = {"vm", NULL};
static const char *test_driver_deps[] = {"allocator", NULL};

/* Target definitions for main build */
static const otter_target_definition targets[] = {
    {"allocator", NULL, allocator_deps, NULL, OTTER_TARGET_OBJECT},
    {"string", NULL, string_deps, NULL, OTTER_TARGET_OBJECT},
    {"array", NULL, array_deps, NULL, OTTER_TARGET_OBJECT},
    {"cstring", NULL, cstring_deps, NULL, OTTER_TARGET_OBJECT},
    {"logger", NULL, logger_deps, NULL, OTTER_TARGET_OBJECT},
    {"process_manager", NULL, process_manager_deps, NULL, OTTER_TARGET_OBJECT},
    {"file", NULL, file_deps, NULL, OTTER_TARGET_OBJECT},
    {"filesystem", NULL, filesystem_deps, NULL, OTTER_TARGET_OBJECT},
    {"target", NULL, target_deps, NULL, OTTER_TARGET_OBJECT},
    {"build", NULL, build_deps, NULL, OTTER_TARGET_OBJECT},
    {"token", NULL, token_deps, NULL, OTTER_TARGET_OBJECT},
    {"node", NULL, node_deps, NULL, OTTER_TARGET_OBJECT},
    {"lexer", NULL, lexer_deps, NULL, OTTER_TARGET_OBJECT},
    {"parser", NULL, parser_deps, NULL, OTTER_TARGET_OBJECT},
    {"bytecode", NULL, bytecode_deps, NULL, OTTER_TARGET_OBJECT},
    {"vm", NULL, vm_deps, NULL, OTTER_TARGET_OBJECT},
    {"test", NULL, test_deps, NULL, OTTER_TARGET_OBJECT},
    {"otter", NULL, otter_exe_deps, NULL, OTTER_TARGET_EXECUTABLE},
    {"test_driver", NULL, test_driver_deps, NULL, OTTER_TARGET_EXECUTABLE},
    {"cstring_tests", NULL, cstring_tests_deps, NULL,
     OTTER_TARGET_SHARED_OBJECT},
    {"string_tests", NULL, string_tests_deps, NULL, OTTER_TARGET_SHARED_OBJECT},
    {"array_tests", NULL, array_tests_deps, NULL, OTTER_TARGET_SHARED_OBJECT},
    {"lexer_tests", NULL, lexer_tests_deps, NULL, OTTER_TARGET_SHARED_OBJECT},
    {"parser_tests", NULL, parser_tests_deps, NULL, OTTER_TARGET_SHARED_OBJECT},
    {"parser_integration_tests", NULL, parser_integration_tests_deps, NULL,
     OTTER_TARGET_SHARED_OBJECT},
    {"build_tests", NULL, build_tests_deps, "-lgnutls",
     OTTER_TARGET_SHARED_OBJECT},
    {"build_tests_extended", NULL, build_tests_extended_deps, "-lgnutls",
     OTTER_TARGET_SHARED_OBJECT},
    {"build_integration_tests", NULL, build_integration_tests_deps, "-lgnutls",
     OTTER_TARGET_SHARED_OBJECT},
    {NULL, NULL, NULL, NULL, OTTER_TARGET_OBJECT}};

static bool build_bootstrap_make(otter_allocator *allocator,
                                 otter_filesystem *filesystem,
                                 otter_logger *logger,
                                 otter_process_manager *process_manager) {
  /* Bootstrap uses a subset of the main targets - just the dependencies
   * needed for otter_make itself */
  static const char *otter_make_deps[] = {
      "allocator", "cstring",         "string", "array", "file", "filesystem",
      "logger",    "process_manager", "target", "build", NULL};

  static const otter_target_definition bootstrap_targets[] = {
      {"allocator", NULL, allocator_deps, NULL, OTTER_TARGET_OBJECT},
      {"string", NULL, string_deps, NULL, OTTER_TARGET_OBJECT},
      {"array", NULL, array_deps, NULL, OTTER_TARGET_OBJECT},
      {"cstring", NULL, cstring_deps, NULL, OTTER_TARGET_OBJECT},
      {"logger", NULL, logger_deps, NULL, OTTER_TARGET_OBJECT},
      {"process_manager", NULL, process_manager_deps, NULL,
       OTTER_TARGET_OBJECT},
      {"file", NULL, file_deps, NULL, OTTER_TARGET_OBJECT},
      {"filesystem", NULL, filesystem_deps, NULL, OTTER_TARGET_OBJECT},
      {"target", NULL, target_deps, NULL, OTTER_TARGET_OBJECT},
      {"build", NULL, build_deps, NULL, OTTER_TARGET_OBJECT},
      {"otter_make", "make", otter_make_deps, "-lgnutls",
       OTTER_TARGET_EXECUTABLE},
      {NULL, NULL, NULL, NULL, OTTER_TARGET_OBJECT}};

  otter_build_config config = {
      .paths =
          {
              .src_dir = "./src",
              .out_dir = "./release",
              .suffix = "",
          },
      .flags =
          {
              .cc_flags = CC_FLAGS_RELEASE,
              .ll_flags = LL_FLAGS_RELEASE,
              .include_flags = CC_INCLUDE_FLAGS,
          },
  };

  OTTER_CLEANUP(otter_build_context_free_p)
  otter_build_context *ctx =
      otter_build_context_create(bootstrap_targets, allocator, filesystem,
                                 logger, process_manager, &config);
  if (ctx == NULL) {
    return false;
  }

  if (!otter_build_all(ctx)) {
    return false;
  }

  /* Move otter_make from release directory to root for convenience */
  if (rename("./release/otter_make", "./otter_make") != 0) {
    otter_log_error(logger, "Failed to move otter_make to root directory");
    return false;
  }

  return true;
}

int main(int argc, char *argv[]) {
  /* Define build modes */
  static const otter_build_mode_config modes[] = {
      {
          .name = "debug",
          .config =
              {
                  .paths = {.src_dir = "./src",
                            .out_dir = "./debug",
                            .suffix = ""},
                  .flags =
                      {
                          .cc_flags = CC_FLAGS_DEBUG,
                          .ll_flags = LL_FLAGS_DEBUG,
                          .include_flags = CC_INCLUDE_FLAGS,
                      },
              },
      },
      {
          .name = "release",
          .config =
              {
                  .paths = {.src_dir = "./src",
                            .out_dir = "./release",
                            .suffix = ""},
                  .flags =
                      {
                          .cc_flags = CC_FLAGS_RELEASE,
                          .ll_flags = LL_FLAGS_RELEASE,
                          .include_flags = CC_INCLUDE_FLAGS,
                      },
              },
      },
  };

  /* Use the generic build driver */
  return otter_build_driver_main(argc, argv, targets, modes, 2, 0,
                                 build_bootstrap_make);
}
