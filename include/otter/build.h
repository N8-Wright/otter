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
#ifndef OTTER_BUILD_H_
#define OTTER_BUILD_H_

#include "allocator.h"
#include "array.h"
#include "filesystem.h"
#include "inc.h"
#include "logger.h"
#include "process_manager.h"
#include "string.h"
#include "target.h"

#include <stdbool.h>
#include <stddef.h>

/**
 * Path configuration for builds
 */
typedef struct {
  const char *src_dir; /* Source directory */
  const char *out_dir; /* Output directory */
  const char *suffix;  /* Output file suffix (e.g., "_coverage") */
} otter_build_paths;

/**
 * Compiler and linker flags
 */
typedef struct {
  const char *cc_flags;      /* Compiler flags */
  const char *ll_flags;      /* Linker flags */
  const char *include_flags; /* Include path flags (e.g., "-I./include") */
} otter_build_flags;

/**
 * Configuration for a build variant (debug, release, coverage, etc.)
 */
typedef struct {
  otter_build_paths paths;
  otter_build_flags flags;
} otter_build_config;

/**
 * Definition of a build target
 */
typedef struct {
  const char *name;   /* Target name (used for lookup and as default source) */
  const char *source; /* Source file name (without extension), NULL uses name */
  const char **deps;  /* NULL-terminated array of dependency names */
  const char *extra_flags; /* Additional flags (optional) */
  otter_target_type type;  /* Target type (object, executable, shared) */
} otter_target_definition;

/**
 * Helper for defining target with no dependencies
 */
#define NO_DEPS ((const char *[]){NULL})

/**
 * Helper macro for defining dependency lists inline
 * Usage: DEPS("dep1", "dep2") expands to (const char *[]){"dep1", "dep2", NULL}
 */
#define DEPS(...) ((const char *[]){__VA_ARGS__, NULL})

/**
 * Helper macros for common target types
 * When source is NULL, the target name is used as the source file name
 */
#define OBJECT_TARGET(name, deps) {name, NULL, deps, NULL, OTTER_TARGET_OBJECT}

#define OBJECT_TARGET_SRC(name, source, deps)                                  \
  {name, source, deps, NULL, OTTER_TARGET_OBJECT}

#define EXECUTABLE_TARGET(name, deps)                                          \
  {name, NULL, deps, NULL, OTTER_TARGET_EXECUTABLE}

#define EXECUTABLE_TARGET_SRC(name, source, deps)                              \
  {name, source, deps, NULL, OTTER_TARGET_EXECUTABLE}

#define SHARED_TARGET(name, deps, flags)                                       \
  {name, NULL, deps, flags, OTTER_TARGET_SHARED_OBJECT}

#define SHARED_TARGET_SRC(name, source, deps, flags)                           \
  {name, source, deps, flags, OTTER_TARGET_SHARED_OBJECT}

/**
 * Sentinel for marking the end of target definition arrays
 */
#define TARGET_LIST_END {NULL, NULL, NULL, NULL, OTTER_TARGET_OBJECT}

/**
 * Build context that manages targets and build state
 * (opaque - internal structure hidden from users)
 */
typedef struct otter_build_context otter_build_context;

/**
 * Create a new build context and build all targets
 *
 * @param target_defs NULL-terminated array of target definitions
 * @param allocator Memory allocator
 * @param filesystem Filesystem interface
 * @param logger Logger
 * @param process_manager Process manager
 * @param config Build configuration
 * @return New build context, or NULL on error
 */
otter_build_context *otter_build_context_create(
    const otter_target_definition *target_defs, otter_allocator *allocator,
    otter_filesystem *filesystem, otter_logger *logger,
    otter_process_manager *process_manager, const otter_build_config *config);

/**
 * Free a build context and all associated resources
 *
 * @param ctx Build context to free
 */
void otter_build_context_free(otter_build_context *ctx);

OTTER_DECLARE_TRIVIAL_CLEANUP_FUNC(otter_build_context *,
                                   otter_build_context_free);

/**
 * Build all targets in the context
 *
 * @param ctx Build context
 * @return true on success, false on error
 */
bool otter_build_all(otter_build_context *ctx);

#endif /* OTTER_BUILD_H_ */
