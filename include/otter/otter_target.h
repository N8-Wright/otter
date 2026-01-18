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
#include "otter_allocator.h"
#include "otter_array.h"
#include "otter_filesystem.h"
#include "otter_inc.h"
#include "otter_logger.h"

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
  char *name;
  otter_target_type type;
  OTTER_ARRAY_DECLARE(char *, files);

  char *command;
  char *cc_flags;
  char *include_flags;
  OTTER_ARRAY_DECLARE(char *, argv);
  OTTER_ARRAY_DECLARE(otter_target *, dependencies);
  unsigned char *hash;
  unsigned int hash_size;
  bool executed;
};

int otter_target_execute(otter_target *target);
void otter_target_free(otter_target *target);
OTTER_DECLARE_TRIVIAL_CLEANUP_FUNC(otter_target *, otter_target_free);
otter_target *otter_target_create_c_object(const char *name, const char *flags,
                                           const char *include_flags,
                                           otter_allocator *allocator,
                                           otter_filesystem *filesystem,
                                           otter_logger *logger, ...);
otter_target *otter_target_create_c_executable(
    const char *name, const char *flags, const char *include_flags,
    otter_allocator *allocator, otter_filesystem *filesystem,
    otter_logger *logger, const char **files, otter_target **dependencies);
otter_target *otter_target_create_c_shared_object(
    const char *name, const char *flags, const char *include_flags,
    otter_allocator *allocator, otter_filesystem *filesystem,
    otter_logger *logger, const char **files, otter_target **dependencies);

void otter_target_add_command(otter_target *target, const char *command);
void otter_target_add_dependency(otter_target *target, otter_target *dep);
#endif /* OTTER_H_ */
