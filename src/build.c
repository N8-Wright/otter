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
#include "otter/allocator.h"
#include "otter/array.h"
#include "otter/filesystem.h"
#include "otter/logger.h"
#include "otter/process_manager.h"
#include "otter/string.h"
#include "otter/target.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

/**
 * Build context that manages targets and build state (internal structure)
 */
struct otter_build_context {
  OTTER_ARRAY_DECLARE(otter_target *, targets);
  const otter_target_definition *target_defs;
  otter_allocator *allocator;
  otter_filesystem *filesystem;
  otter_logger *logger;
  otter_process_manager *process_manager;
  const otter_build_config *config;
  otter_string *cc_flags_str;
  otter_string *include_flags_str;
  otter_string *exe_flags_str;
};

static const char *get_extension_for_type(otter_target_type type) {
  switch (type) {
  case OTTER_TARGET_OBJECT:
    return ".o";
  case OTTER_TARGET_EXECUTABLE:
    return "";
  case OTTER_TARGET_SHARED_OBJECT:
    return ".so";
  default:
    return "";
  }
}

static otter_string *create_path(otter_allocator *allocator, const char *dir,
                                 const char *name, const char *suffix,
                                 const char *ext) {
  return otter_string_format(allocator, "%s/%s%s%s", dir, name, suffix, ext);
}

static otter_target *find_target_by_name(const otter_build_context *ctx,
                                         const char *name) {
  if (ctx == NULL || name == NULL) {
    return NULL;
  }

  for (size_t i = 0;
       i < OTTER_ARRAY_LENGTH(ctx, targets) && ctx->target_defs[i].name != NULL;
       i++) {
    if (strcmp(ctx->target_defs[i].name, name) == 0) {
      return OTTER_ARRAY_AT_UNSAFE(ctx, targets, i);
    }
  }
  return NULL;
}

otter_build_context *otter_build_context_create(
    const otter_target_definition *target_defs, otter_allocator *allocator,
    otter_filesystem *filesystem, otter_logger *logger,
    otter_process_manager *process_manager, const otter_build_config *config) {

  if (target_defs == NULL || allocator == NULL || filesystem == NULL ||
      logger == NULL || process_manager == NULL || config == NULL) {
    return NULL;
  }

  otter_build_context *ctx =
      otter_malloc(allocator, sizeof(otter_build_context));
  if (ctx == NULL) {
    return NULL;
  }

  ctx->target_defs = target_defs;
  ctx->allocator = allocator;
  ctx->filesystem = filesystem;
  ctx->logger = logger;
  ctx->process_manager = process_manager;
  ctx->config = config;

  /* Initialize string pointers to NULL for safe cleanup */
  ctx->cc_flags_str = NULL;
  ctx->include_flags_str = NULL;
  ctx->exe_flags_str = NULL;

  OTTER_ARRAY_INIT(ctx, targets, allocator);

  /* Create compiler flags string */
  ctx->cc_flags_str = otter_string_from_cstr(allocator, config->flags.cc_flags);
  if (ctx->cc_flags_str == NULL) {
    otter_build_context_free(ctx);
    return NULL;
  }

  /* Create include flags string */
  ctx->include_flags_str = otter_string_from_cstr(
      allocator,
      config->flags.include_flags != NULL ? config->flags.include_flags : "");
  if (ctx->include_flags_str == NULL) {
    otter_build_context_free(ctx);
    return NULL;
  }

  /* Create executable/linker flags string */
  ctx->exe_flags_str = otter_string_format(allocator, "%s%s",
                                           otter_string_cstr(ctx->cc_flags_str),
                                           config->flags.ll_flags);
  if (ctx->exe_flags_str == NULL) {
    otter_build_context_free(ctx);
    return NULL;
  }

  return ctx;
}

void otter_build_context_free(otter_build_context *ctx) {
  if (ctx == NULL) {
    return;
  }

  /* Free all targets */
  for (size_t i = 0; i < OTTER_ARRAY_LENGTH(ctx, targets); i++) {
    otter_target_free(OTTER_ARRAY_AT_UNSAFE(ctx, targets, i));
  }
  otter_free(ctx->allocator, ctx->targets);

  /* Free flag strings */
  if (ctx->cc_flags_str != NULL) {
    otter_string_free(ctx->cc_flags_str);
  }
  if (ctx->include_flags_str != NULL) {
    otter_string_free(ctx->include_flags_str);
  }
  if (ctx->exe_flags_str != NULL) {
    otter_string_free(ctx->exe_flags_str);
  }

  otter_free(ctx->allocator, ctx);
}

/**
 * Helper to get flags for a target, including any extra flags
 */
static const otter_string *
get_flags_for_target(otter_build_context *ctx,
                     const otter_target_definition *def,
                     otter_string **out_allocated_flags) {

  *out_allocated_flags = NULL;

  /* Object files use compiler flags only */
  if (def->type == OTTER_TARGET_OBJECT) {
    return ctx->cc_flags_str;
  }

  /* Executables and shared objects use combined flags */
  if (def->extra_flags == NULL) {
    return ctx->exe_flags_str;
  }

  /* Append extra flags with a space separator */
  *out_allocated_flags = otter_string_format(
      ctx->allocator, "%s %s", otter_string_cstr(ctx->exe_flags_str),
      def->extra_flags);

  return *out_allocated_flags;
}

/**
 * Helper to resolve dependencies by name and add them to a target
 */
static bool add_dependencies_to_target(otter_build_context *ctx,
                                       otter_target *target,
                                       const otter_target_definition *def) {
  if (def->deps == NULL) {
    return true;
  }

  for (size_t j = 0; def->deps[j] != NULL; j++) {
    otter_target *dep = find_target_by_name(ctx, def->deps[j]);
    if (dep == NULL) {
      otter_log_error(ctx->logger, "Dependency '%s' not found for target '%s'",
                      def->deps[j], def->name);
      return false;
    }
    otter_target_add_dependency(target, dep);
  }
  return true;
}

/**
 * Helper to create dependency array for executables/shared objects
 */
static otter_target **
create_dependency_array(otter_build_context *ctx,
                        const otter_target_definition *def, size_t *out_count) {
  /* Count dependencies */
  size_t dep_count = 0;
  if (def->deps != NULL) {
    while (def->deps[dep_count] != NULL) {
      dep_count++;
    }
  }
  *out_count = dep_count;

  if (dep_count == 0) {
    return NULL;
  }

  /* Allocate and populate array */
  otter_target **deps =
      otter_malloc(ctx->allocator, sizeof(otter_target *) * (dep_count + 1));
  if (deps == NULL) {
    return NULL;
  }

  for (size_t j = 0; j < dep_count; j++) {
    deps[j] = find_target_by_name(ctx, def->deps[j]);
    if (deps[j] == NULL) {
      otter_log_error(ctx->logger, "Dependency '%s' not found for target '%s'",
                      def->deps[j], def->name);
      otter_free(ctx->allocator, deps);
      return NULL;
    }
  }
  deps[dep_count] = NULL;

  return deps;
}

/**
 * Create a single target from its definition
 */
static otter_target *create_target(otter_build_context *ctx,
                                   const otter_target_definition *def) {
  /* Get extension from target type */
  const char *extension = get_extension_for_type(def->type);

  /* Use name as source if source is NULL */
  const char *source_name = def->source != NULL ? def->source : def->name;

  /* Create output file path */
  OTTER_CLEANUP(otter_string_free_p)
  otter_string *output_file =
      create_path(ctx->allocator, ctx->config->paths.out_dir, def->name,
                  ctx->config->paths.suffix, extension);
  if (output_file == NULL) {
    return NULL;
  }

  /* Create source file path */
  OTTER_CLEANUP(otter_string_free_p)
  otter_string *src_file = create_path(
      ctx->allocator, ctx->config->paths.src_dir, source_name, "", ".c");
  if (src_file == NULL) {
    return NULL;
  }

  /* Get flags for this target */
  OTTER_CLEANUP(otter_string_free_p)
  otter_string *allocated_flags = NULL;
  const otter_string *flags = get_flags_for_target(ctx, def, &allocated_flags);
  if (flags == NULL) {
    return NULL;
  }

  /* Create target based on type */
  otter_target *target = NULL;

  switch (def->type) {
  case OTTER_TARGET_OBJECT:
    target = otter_target_create_c_object(
        output_file, flags, ctx->include_flags_str, ctx->allocator,
        ctx->filesystem, ctx->logger, ctx->process_manager, src_file, NULL);
    break;

  case OTTER_TARGET_EXECUTABLE:
  case OTTER_TARGET_SHARED_OBJECT: {
    /* Create dependency array for linked targets */
    size_t dep_count;
    otter_target **deps = create_dependency_array(ctx, def, &dep_count);

    /* deps can be NULL if there are no dependencies, which is valid */
    if (def->deps != NULL && deps == NULL) {
      return NULL; /* Error already logged */
    }

    /* Create the target */
    if (def->type == OTTER_TARGET_EXECUTABLE) {
      target = otter_target_create_c_executable(
          output_file, flags, ctx->include_flags_str, ctx->allocator,
          ctx->filesystem, ctx->logger, ctx->process_manager,
          (const otter_string *[]){src_file, NULL}, deps);
    } else {
      target = otter_target_create_c_shared_object(
          output_file, flags, ctx->include_flags_str, ctx->allocator,
          ctx->filesystem, ctx->logger, ctx->process_manager,
          (const otter_string *[]){src_file, NULL}, deps);
    }

    /* Free the deps array (ownership passed to target) */
    if (deps != NULL) {
      otter_free(ctx->allocator, deps);
    }
    break;
  }
  }

  return target;
}

static bool create_targets(otter_build_context *ctx) {
  if (ctx == NULL) {
    return false;
  }

  /* First pass: create all targets */
  for (size_t i = 0; ctx->target_defs[i].name != NULL; i++) {
    const otter_target_definition *def = &ctx->target_defs[i];

    otter_target *target = create_target(ctx, def);
    if (target == NULL) {
      return false;
    }

    if (!OTTER_ARRAY_APPEND(ctx, targets, ctx->allocator, target)) {
      otter_target_free(target);
      return false;
    }
  }

  /* Second pass: add dependencies for object files */
  for (size_t i = 0; ctx->target_defs[i].name != NULL; i++) {
    const otter_target_definition *def = &ctx->target_defs[i];
    otter_target *target = OTTER_ARRAY_AT_UNSAFE(ctx, targets, i);

    if (def->type == OTTER_TARGET_OBJECT) {
      /* Object files need dependencies added after all targets exist */
      if (!add_dependencies_to_target(ctx, target, def)) {
        return false;
      }
    }
  }

  /* Third pass: execute all targets */
  for (size_t i = 0; ctx->target_defs[i].name != NULL; i++) {
    otter_target *target = OTTER_ARRAY_AT_UNSAFE(ctx, targets, i);
    int result = otter_target_execute(target);
    if (result != 0) {
      return false;
    }
  }

  return true;
}

/**
 * Find target definition index by name
 * Returns -1 if not found
 */
static int find_target_def_index(const otter_build_context *ctx,
                                 const char *name) {
  for (size_t i = 0; ctx->target_defs[i].name != NULL; i++) {
    if (strcmp(ctx->target_defs[i].name, name) == 0) {
      return (int)i;
    }
  }
  return -1;
}

/**
 * Detect circular dependencies using DFS
 * Returns true if a cycle is detected
 */
static bool has_circular_dependency_dfs(const otter_build_context *ctx,
                                        int current_idx, bool *visiting,
                                        bool *visited, const char **cycle_path,
                                        size_t *path_len) {

  if (current_idx < 0) {
    return false;
  }

  /* Already visiting this node - cycle detected */
  if (visiting[current_idx]) {
    cycle_path[(*path_len)++] = ctx->target_defs[current_idx].name;
    return true;
  }

  /* Already fully visited */
  if (visited[current_idx]) {
    return false;
  }

  /* Mark as visiting */
  visiting[current_idx] = true;
  cycle_path[(*path_len)++] = ctx->target_defs[current_idx].name;

  /* Visit all dependencies */
  const otter_target_definition *def = &ctx->target_defs[current_idx];
  if (def->deps != NULL) {
    for (size_t i = 0; def->deps[i] != NULL; i++) {
      int dep_idx = find_target_def_index(ctx, def->deps[i]);
      if (dep_idx < 0) {
        /* Dependency not found - will be caught elsewhere */
        continue;
      }

      if (has_circular_dependency_dfs(ctx, dep_idx, visiting, visited,
                                      cycle_path, path_len)) {
        return true;
      }
    }
  }

  /* Done visiting this node */
  visiting[current_idx] = false;
  visited[current_idx] = true;
  (*path_len)--;
  return false;
}

/**
 * Check for circular dependencies in target definitions
 */
static bool check_circular_dependencies(const otter_build_context *ctx,
                                        size_t target_count) {
  /* Allocate tracking arrays */
  bool *visiting = otter_malloc(ctx->allocator, sizeof(bool) * target_count);
  bool *visited = otter_malloc(ctx->allocator, sizeof(bool) * target_count);
  const char **cycle_path =
      otter_malloc(ctx->allocator, sizeof(char *) * (target_count + 1));

  if (visiting == NULL || visited == NULL || cycle_path == NULL) {
    otter_free(ctx->allocator, visiting);
    otter_free(ctx->allocator, visited);
    otter_free(ctx->allocator, cycle_path);
    return false;
  }

  /* Initialize arrays */
  for (size_t i = 0; i < target_count; i++) {
    visiting[i] = false;
    visited[i] = false;
  }

  /* Run DFS from each unvisited node */
  bool has_cycle = false;
  for (size_t i = 0; i < target_count && !has_cycle; i++) {
    if (!visited[i]) {
      size_t path_len = 0;
      if (has_circular_dependency_dfs(ctx, (int)i, visiting, visited,
                                      cycle_path, &path_len)) {
        /* Log the cycle */
        otter_log_error(ctx->logger, "Circular dependency detected:");
        for (size_t j = 0; j < path_len; j++) {
          otter_log_error(ctx->logger, "  %s ->", cycle_path[j]);
        }
        otter_log_error(ctx->logger, "  %s (cycle)", cycle_path[0]);
        has_cycle = true;
      }
    }
  }

  /* Cleanup */
  otter_free(ctx->allocator, visiting);
  otter_free(ctx->allocator, visited);
  otter_free(ctx->allocator, cycle_path);

  return has_cycle;
}

/**
 * Validate that all dependencies exist
 */
static bool validate_dependencies_exist(const otter_build_context *ctx) {
  for (size_t i = 0; ctx->target_defs[i].name != NULL; i++) {
    const otter_target_definition *def = &ctx->target_defs[i];
    if (def->deps != NULL) {
      for (size_t j = 0; def->deps[j] != NULL; j++) {
        if (find_target_def_index(ctx, def->deps[j]) < 0) {
          otter_log_error(ctx->logger,
                          "Target '%s' depends on undefined target '%s'",
                          def->name, def->deps[j]);
          return false;
        }
      }
    }
  }
  return true;
}

/**
 * Validate target definitions before building
 */
static bool validate_target_definitions(const otter_build_context *ctx) {
  /* Count targets */
  size_t target_count = 0;
  while (ctx->target_defs[target_count].name != NULL) {
    target_count++;
  }

  /* Check for duplicate target names */
  for (size_t i = 0; i < target_count; i++) {
    const char *name = ctx->target_defs[i].name;
    for (size_t j = i + 1; j < target_count; j++) {
      if (strcmp(name, ctx->target_defs[j].name) == 0) {
        otter_log_error(ctx->logger, "Duplicate target name: '%s'", name);
        return false;
      }
    }
  }

  /* Check that all dependencies exist */
  if (!validate_dependencies_exist(ctx)) {
    return false;
  }

  /* Check for circular dependencies */
  if (check_circular_dependencies(ctx, target_count)) {
    return false;
  }

  return true;
}

bool otter_build_all(otter_build_context *ctx) {
  if (ctx == NULL) {
    return false;
  }

  /* Validate target definitions first */
  if (!validate_target_definitions(ctx)) {
    return false;
  }

  return create_targets(ctx);
}
