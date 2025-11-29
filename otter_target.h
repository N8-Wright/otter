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
#include "otter_filesystem.h"

#include <stdbool.h>
#include <stddef.h>
#define OTTER_XATTR_NAME "user.otter-sha1"
#ifdef __linux__
#define OTTER_CC "cc"
#elif _WIN32
#define OTTER_CC "cl"
#endif

typedef struct otter_target otter_target;
typedef struct otter_dependency {
  otter_target *target;
  struct otter_dependency *next;
} otter_dependency;

struct otter_target {
  otter_allocator *allocator;
  otter_filesystem *filesystem;
  char *name;

  size_t files_length;
  size_t files_capacity;
  char **files;

  size_t argv_length;
  size_t argv_capacity;
  char **argv;
  otter_dependency *dependencies;
};

int otter_target_execute(otter_target *target);
void otter_target_free(otter_target *target);
otter_target *otter_target_create(const char *name, otter_allocator *allocator,
                                  otter_filesystem *filesystem, ...);
void otter_target_add_command(otter_target *target, const char *command);
void otter_target_add_dependency(otter_target *target, otter_target *dep);

void otter_target_argv_insert(otter_target *target, char *arg);
bool otter_target_files_insert(otter_target *target, char *arg);

bool otter_target_generate_hash(otter_target *target, unsigned char *hash,
                                unsigned int *hash_size);
bool otter_target_needs_execute(otter_target *target);
void otter_target_store_hash(otter_target *target);
#endif /* OTTER_H_ */
