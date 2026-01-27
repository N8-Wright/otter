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
#ifndef OTTER_H_
#define OTTER_H_
#include "allocator.h"
#include "array.h"
#include "filesystem.h"
#include "inc.h"
#include "logger.h"
#include "process_manager.h"
#include "string.h"

#include <stdbool.h>
#include <stddef.h>
#define OTTER_XATTR_NAME "user.otter-sha1"
#ifdef __linux__
#define OTTER_CC "cc"
#elif _WIN32
#define OTTER_CC "cl"
#endif

typedef enum otter_target_type {
  OTTER_TARGET_OBJECT,        /* .o files */
  OTTER_TARGET_SHARED_OBJECT, /* .so files */
  OTTER_TARGET_EXECUTABLE,
} otter_target_type;

typedef struct otter_target otter_target;
struct otter_target {
  otter_allocator *allocator;
  otter_filesystem *filesystem;
  otter_logger *logger;
  otter_process_manager *process_manager;
  otter_string *name;
  otter_target_type type;
  OTTER_ARRAY_DECLARE(otter_string *, files);

  otter_string *command;
  otter_string *cc_flags;
  otter_string *include_flags;
  OTTER_ARRAY_DECLARE(otter_string *, argv);
  OTTER_ARRAY_DECLARE(otter_target *, dependencies);
  unsigned char *hash;
  unsigned int hash_size;
  bool executed;
};

int otter_target_execute(otter_target *target);
void otter_target_free(otter_target *target);
OTTER_DECLARE_TRIVIAL_CLEANUP_FUNC(otter_target *, otter_target_free);
otter_target *otter_target_create_c_object(
    const otter_string *name, const otter_string *flags,
    const otter_string *include_flags, otter_allocator *allocator,
    otter_filesystem *filesystem, otter_logger *logger,
    otter_process_manager *process_manager, ...);
otter_target *otter_target_create_c_executable(
    const otter_string *name, const otter_string *flags,
    const otter_string *include_flags, otter_allocator *allocator,
    otter_filesystem *filesystem, otter_logger *logger,
    otter_process_manager *process_manager, const otter_string **files,
    otter_target **dependencies);
otter_target *otter_target_create_c_shared_object(
    const otter_string *name, const otter_string *flags,
    const otter_string *include_flags, otter_allocator *allocator,
    otter_filesystem *filesystem, otter_logger *logger,
    otter_process_manager *process_manager, const otter_string **files,
    otter_target **dependencies);

void otter_target_add_command(otter_target *target,
                              const otter_string *command);
void otter_target_add_dependency(otter_target *target, otter_target *dep);
#endif /* OTTER_H_ */
