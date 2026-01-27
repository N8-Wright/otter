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
#include "otter/array.h"
#include "otter/cstring.h"
#include "otter/filesystem.h"
#include "otter/inc.h"
#include "otter/logger.h"
#include "otter/process_manager.h"
#include "otter/string.h"
#include "otter/target.h"

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

typedef struct {
  const char *name;
  const char *source;
  const char **deps;
  const char *extra_flags;
  otter_target_type type;
  const char *extension;
} target_definition;

typedef struct {
  OTTER_ARRAY_DECLARE(otter_target *, targets);
  const target_definition *target_defs;
  otter_allocator *allocator;
  otter_filesystem *filesystem;
  otter_logger *logger;
  otter_process_manager *process_manager;
  const build_config *config;
  const otter_string *cc_flags_str;
  const otter_string *include_flags_str;
  const otter_string *exe_flags_str;
} build_context;

static otter_string *create_path(otter_allocator *allocator, const char *dir,
                                 const char *name, const char *suffix,
                                 const char *ext) {
  return otter_string_format(allocator, "%s/%s%s%s", dir, name, suffix, ext);
}

static otter_target *find_target_by_name(const build_context *ctx,
                                         const char *name) {
  for (size_t i = 0;
       i < OTTER_ARRAY_LENGTH(ctx, targets) && ctx->target_defs[i].name != NULL;
       i++) {
    if (strcmp(ctx->target_defs[i].name, name) == 0) {
      return OTTER_ARRAY_AT_UNSAFE(ctx, targets, i);
    }
  }
  return NULL;
}

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
static const char *cstring_tests_deps[] = {"test", "cstring", NULL};
static const char *string_tests_deps[] = {"test", "string", NULL};
static const char *array_tests_deps[] = {"test", "array", NULL};
static const char *lexer_tests_deps[] = {"test", "lexer", "token", NULL};
static const char *parser_tests_deps[] = {"test", "cstring", "node", "parser",
                                          NULL};
static const char *parser_integration_tests_deps[] = {"test", "lexer", "node",
                                                      "parser", NULL};
static const char *otter_exe_deps[] = {"vm", NULL};
static const char *test_driver_deps[] = {"allocator", NULL};

static const target_definition targets[] = {
    {"allocator", "allocator", allocator_deps, NULL, OTTER_TARGET_OBJECT, ".o"},
    {"string", "string", string_deps, NULL, OTTER_TARGET_OBJECT, ".o"},
    {"array", "array", array_deps, NULL, OTTER_TARGET_OBJECT, ".o"},
    {"cstring", "cstring", cstring_deps, NULL, OTTER_TARGET_OBJECT, ".o"},
    {"logger", "logger", logger_deps, NULL, OTTER_TARGET_OBJECT, ".o"},
    {"process_manager", "process_manager", process_manager_deps, NULL,
     OTTER_TARGET_OBJECT, ".o"},
    {"file", "file", file_deps, NULL, OTTER_TARGET_OBJECT, ".o"},
    {"filesystem", "filesystem", filesystem_deps, NULL, OTTER_TARGET_OBJECT,
     ".o"},
    {"target", "target", target_deps, NULL, OTTER_TARGET_OBJECT, ".o"},
    {"token", "token", token_deps, NULL, OTTER_TARGET_OBJECT, ".o"},
    {"node", "node", node_deps, NULL, OTTER_TARGET_OBJECT, ".o"},
    {"lexer", "lexer", lexer_deps, NULL, OTTER_TARGET_OBJECT, ".o"},
    {"parser", "parser", parser_deps, NULL, OTTER_TARGET_OBJECT, ".o"},
    {"bytecode", "bytecode", bytecode_deps, NULL, OTTER_TARGET_OBJECT, ".o"},
    {"vm", "vm", vm_deps, NULL, OTTER_TARGET_OBJECT, ".o"},
    {"test", "test", test_deps, NULL, OTTER_TARGET_OBJECT, ".o"},
    {"otter", "otter", otter_exe_deps, NULL, OTTER_TARGET_EXECUTABLE, ""},
    {"test_driver", "test_driver", test_driver_deps, NULL,
     OTTER_TARGET_EXECUTABLE, ""},
    {"cstring_tests", "cstring_tests", cstring_tests_deps, NULL,
     OTTER_TARGET_SHARED_OBJECT, ".so"},
    {"string_tests", "string_tests", string_tests_deps, NULL,
     OTTER_TARGET_SHARED_OBJECT, ".so"},
    {"array_tests", "array_tests", array_tests_deps, NULL,
     OTTER_TARGET_SHARED_OBJECT, ".so"},
    {"lexer_tests", "lexer_tests", lexer_tests_deps, NULL,
     OTTER_TARGET_SHARED_OBJECT, ".so"},
    {"parser_tests", "parser_tests", parser_tests_deps, NULL,
     OTTER_TARGET_SHARED_OBJECT, ".so"},
    {"parser_integration_tests", "parser_integration_tests",
     parser_integration_tests_deps, NULL, OTTER_TARGET_SHARED_OBJECT, ".so"},
    {NULL, NULL, NULL, NULL, OTTER_TARGET_OBJECT, NULL}};

static bool create_targets(build_context *ctx) {
  for (size_t i = 0; ctx->target_defs[i].name != NULL; i++) {
    const target_definition *def = &ctx->target_defs[i];

    OTTER_CLEANUP(otter_string_free_p)
    otter_string *output_file =
        create_path(ctx->allocator, ctx->config->out_dir, def->name,
                    ctx->config->suffix, def->extension);
    if (output_file == NULL) {
      return false;
    }

    OTTER_CLEANUP(otter_string_free_p)
    otter_string *src_file = create_path(ctx->allocator, ctx->config->src_dir,
                                         def->source, "", ".c");
    if (src_file == NULL) {
      return false;
    }

    const otter_string *flags = ctx->cc_flags_str;
    OTTER_CLEANUP(otter_string_free_p)
    otter_string *extra_flags = NULL;

    if (def->type == OTTER_TARGET_EXECUTABLE ||
        def->type == OTTER_TARGET_SHARED_OBJECT) {
      flags = ctx->exe_flags_str;
      if (def->extra_flags != NULL) {
        extra_flags = otter_string_format(ctx->allocator, "%s %s",
                                          otter_string_cstr(ctx->exe_flags_str),
                                          def->extra_flags);
        if (extra_flags == NULL) {
          return false;
        }
        flags = extra_flags;
      }
    }

    size_t dep_count = 0;
    while (def->deps[dep_count] != NULL) {
      dep_count++;
    }

    otter_target **deps =
        otter_malloc(ctx->allocator, sizeof(otter_target *) * (dep_count + 1));
    if (deps == NULL) {
      return false;
    }

    for (size_t j = 0; j < dep_count; j++) {
      deps[j] = find_target_by_name(ctx, def->deps[j]);
    }
    deps[dep_count] = NULL;

    otter_target *target = NULL;
    switch (def->type) {
    case OTTER_TARGET_OBJECT:
      target = otter_target_create_c_object(
          output_file, flags, ctx->include_flags_str, ctx->allocator,
          ctx->filesystem, ctx->logger, ctx->process_manager, src_file, NULL);
      break;
    case OTTER_TARGET_EXECUTABLE:
      target = otter_target_create_c_executable(
          output_file, flags, ctx->include_flags_str, ctx->allocator,
          ctx->filesystem, ctx->logger, ctx->process_manager,
          (const otter_string *[]){src_file, NULL}, deps);
      break;
    case OTTER_TARGET_SHARED_OBJECT:
      target = otter_target_create_c_shared_object(
          output_file, flags, ctx->include_flags_str, ctx->allocator,
          ctx->filesystem, ctx->logger, ctx->process_manager,
          (const otter_string *[]){src_file, NULL}, deps);
      break;
    }

    if (target == NULL) {
      return false;
    }

    if (!OTTER_ARRAY_APPEND(ctx, targets, ctx->allocator, target)) {
      return false;
    }

    if (def->type == OTTER_TARGET_OBJECT) {
      for (size_t j = 0; def->deps[j] != NULL; j++) {
        otter_target *dep = find_target_by_name(ctx, def->deps[j]);
        if (dep != NULL) {
          otter_target_add_dependency(target, dep);
        }
      }
    } else {
      otter_target_execute(target);
    }
  }
  return true;
}

static void print_usage(const char *prog) {
  fprintf(stderr, "Usage: %s [--debug | --release]\n", prog);
  fprintf(
      stderr,
      "  --debug    Build with debug flags, ASAN, and coverage (default)\n");
  fprintf(stderr, "  --release  Build with optimized release flags\n");
}

static bool build_bootstrap_make(otter_allocator *allocator,
                                 otter_filesystem *filesystem,
                                 otter_logger *logger,
                                 otter_process_manager *process_manager) {
  build_config bootstrap_obj_config = {"", CC_FLAGS_RELEASE, LL_FLAGS_RELEASE,
                                       "./src", "./release"};
  build_config bootstrap_exe_config = {"", CC_FLAGS_RELEASE, LL_FLAGS_RELEASE,
                                       "./src", "./"};

  OTTER_CLEANUP(otter_string_free_p)
  otter_string *cc_flags_str =
      otter_string_from_cstr(allocator, CC_FLAGS_RELEASE);
  if (cc_flags_str == NULL) {
    return false;
  }

  OTTER_CLEANUP(otter_string_free_p)
  otter_string *include_flags_str =
      otter_string_from_cstr(allocator, CC_INCLUDE_FLAGS);
  if (include_flags_str == NULL) {
    return false;
  }

  OTTER_CLEANUP(otter_string_free_p)
  otter_string *exe_flags_str =
      otter_string_format(allocator, "%s -lgnutls", CC_FLAGS_RELEASE);
  if (exe_flags_str == NULL) {
    return false;
  }

  static const char *bootstrap_allocator_deps[] = {NULL};
  static const char *bootstrap_string_deps[] = {"allocator", NULL};
  static const char *bootstrap_array_deps[] = {"allocator", NULL};
  static const char *bootstrap_cstring_deps[] = {"allocator", NULL};
  static const char *bootstrap_logger_deps[] = {"cstring", "array", "allocator",
                                                NULL};
  static const char *bootstrap_process_manager_deps[] = {"allocator", "logger",
                                                         "string", NULL};
  static const char *bootstrap_file_deps[] = {NULL};
  static const char *bootstrap_filesystem_deps[] = {"file", "allocator", NULL};
  static const char *bootstrap_target_deps[] = {
      "allocator", "array",           "filesystem", "logger",
      "string",    "process_manager", NULL};
  static const char *otter_make_deps[] = {"allocator", "filesystem", "logger",
                                          "target", NULL};

  static const target_definition bootstrap_obj_targets[] = {
      {"allocator", "allocator", bootstrap_allocator_deps, NULL,
       OTTER_TARGET_OBJECT, ".o"},
      {"string", "string", bootstrap_string_deps, NULL, OTTER_TARGET_OBJECT,
       ".o"},
      {"array", "array", bootstrap_array_deps, NULL, OTTER_TARGET_OBJECT, ".o"},
      {"cstring", "cstring", bootstrap_cstring_deps, NULL, OTTER_TARGET_OBJECT,
       ".o"},
      {"logger", "logger", bootstrap_logger_deps, NULL, OTTER_TARGET_OBJECT,
       ".o"},
      {"process_manager", "process_manager", bootstrap_process_manager_deps,
       NULL, OTTER_TARGET_OBJECT, ".o"},
      {"file", "file", bootstrap_file_deps, NULL, OTTER_TARGET_OBJECT, ".o"},
      {"filesystem", "filesystem", bootstrap_filesystem_deps, NULL,
       OTTER_TARGET_OBJECT, ".o"},
      {"target", "target", bootstrap_target_deps, NULL, OTTER_TARGET_OBJECT,
       ".o"},
      {NULL, NULL, NULL, NULL, OTTER_TARGET_OBJECT, NULL}};

  static const target_definition bootstrap_exe_targets[] = {
      {"otter_make", "make", otter_make_deps, NULL, OTTER_TARGET_EXECUTABLE,
       ""},
      {NULL, NULL, NULL, NULL, OTTER_TARGET_OBJECT, NULL}};

  build_context obj_ctx = {
      .target_defs = bootstrap_obj_targets,
      .allocator = allocator,
      .filesystem = filesystem,
      .logger = logger,
      .process_manager = process_manager,
      .config = &bootstrap_obj_config,
      .cc_flags_str = cc_flags_str,
      .include_flags_str = include_flags_str,
      .exe_flags_str = NULL,
  };

  OTTER_ARRAY_INIT(&obj_ctx, targets, allocator);

  if (!create_targets(&obj_ctx)) {
    return false;
  }

  build_context exe_ctx = {
      .target_defs = bootstrap_exe_targets,
      .allocator = allocator,
      .filesystem = filesystem,
      .logger = logger,
      .process_manager = process_manager,
      .config = &bootstrap_exe_config,
      .cc_flags_str = cc_flags_str,
      .include_flags_str = include_flags_str,
      .exe_flags_str = exe_flags_str,
  };

  exe_ctx.targets = obj_ctx.targets;
  exe_ctx.targets_length = obj_ctx.targets_length;
  exe_ctx.targets_capacity = obj_ctx.targets_capacity;

  if (!create_targets(&exe_ctx)) {
    return false;
  }

  return true;
}

static void build_program_and_tests(otter_allocator *allocator,
                                    otter_filesystem *filesystem,
                                    otter_logger *logger,
                                    const build_config *config) {
  OTTER_CLEANUP(otter_process_manager_free_p)
  otter_process_manager *process_manager =
      otter_process_manager_create(allocator, logger);
  if (process_manager == NULL) {
    otter_log_critical(logger, "Failed to create process manager");
    return;
  }

  OTTER_CLEANUP(otter_string_free_p)
  otter_string *cc_flags_str =
      otter_string_from_cstr(allocator, config->cc_flags);
  if (cc_flags_str == NULL) {
    return;
  }

  OTTER_CLEANUP(otter_string_free_p)
  otter_string *include_flags_str =
      otter_string_from_cstr(allocator, CC_INCLUDE_FLAGS);
  if (include_flags_str == NULL) {
    return;
  }

  OTTER_CLEANUP(otter_string_free_p)
  otter_string *exe_flags = otter_string_format(
      allocator, "%s%s", otter_string_cstr(cc_flags_str), config->ll_flags);
  if (exe_flags == NULL) {
    return;
  }

  build_context ctx = {
      .target_defs = targets,
      .allocator = allocator,
      .filesystem = filesystem,
      .logger = logger,
      .process_manager = process_manager,
      .config = config,
      .cc_flags_str = cc_flags_str,
      .include_flags_str = include_flags_str,
      .exe_flags_str = exe_flags,
  };

  OTTER_ARRAY_INIT(&ctx, targets, allocator);

  if (!create_targets(&ctx)) {
    return;
  }
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

  OTTER_CLEANUP(otter_process_manager_free_p)
  otter_process_manager *process_manager =
      otter_process_manager_create(allocator, logger);
  if (process_manager == NULL) {
    otter_log_critical(logger, "Failed to create process manager");
    return 1;
  }

  OTTER_CLEANUP(otter_filesystem_free_p)
  otter_filesystem *filesystem = otter_filesystem_create(allocator);

  if (!build_bootstrap_make(allocator, filesystem, logger, process_manager)) {
    return 1;
  }

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

  return 0;
}
