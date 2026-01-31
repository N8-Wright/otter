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
#ifndef OTTER_FILE_H_
#define OTTER_FILE_H_
#include "inc.h"
#include <stdbool.h>
#include <stddef.h>

#ifdef OTTER_IS_LINUX
#include <sys/stat.h>
#endif

typedef struct otter_file_info {
#ifdef OTTER_IS_LINUX
  struct stat value;
#endif
} otter_file_info;

typedef struct otter_file_owner {
#ifdef OTTER_IS_LINUX
  gid_t gid;
  uid_t uid;
#endif
} otter_file_owner;

typedef struct otter_file_permissions {
#ifdef OTTER_IS_LINUX
  mode_t mode;
#endif
} otter_file_permissions;

otter_file_owner otter_file_info_get_owner(const otter_file_info *info);
otter_file_permissions
otter_file_info_get_permissions(const otter_file_info *info);

typedef struct otter_file otter_file;
typedef struct otter_file_vtable {
  size_t (*read)(otter_file *, void *buffer, size_t num_bytes);
  size_t (*write)(otter_file *, const void *buffer, size_t num_bytes);
  void (*close)(otter_file *);
  bool (*stat)(otter_file *, otter_file_info *stat);
  bool (*set_owner)(otter_file *, const otter_file_owner *owner);
  bool (*set_permissions)(otter_file *,
                          const otter_file_permissions *permissions);
} otter_file_vtable;

struct otter_file {
  otter_file_vtable *vtable;
};

size_t otter_file_read(otter_file *file, void *buffer, size_t num_bytes);
size_t otter_file_write(otter_file *file, const void *buffer, size_t num_bytes);
void otter_file_close(otter_file *file);
bool otter_file_stat(otter_file *file, otter_file_info *stat);
bool otter_file_set_owner(otter_file *file, const otter_file_owner *owner);
bool otter_file_set_permissions(otter_file *file,
                                const otter_file_permissions *permissions);
OTTER_DECLARE_TRIVIAL_CLEANUP_FUNC(otter_file *, otter_file_close);
#endif /* OTTER_FILE_H_ */
