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
#include "otter/file.h"
otter_file_owner otter_file_info_get_owner(const otter_file_info *info) {
  otter_file_owner owner;
#ifdef OTTER_IS_LINUX
  owner.gid = info->value.st_gid;
  owner.uid = info->value.st_uid;
#endif

  return owner;
}

otter_file_permissions
otter_file_info_get_permissions(const otter_file_info *info) {
  otter_file_permissions perm;
#ifdef OTTER_IS_LINUX
  perm.mode = info->value.st_mode;
#endif

  return perm;
}

size_t otter_file_read(otter_file *file, void *buffer, size_t num_bytes) {
  return file->vtable->read(file, buffer, num_bytes);
}

size_t otter_file_write(otter_file *file, const void *buffer,
                        size_t num_bytes) {
  return file->vtable->write(file, buffer, num_bytes);
}

void otter_file_close(otter_file *file) { file->vtable->close(file); }

bool otter_file_stat(otter_file *file, otter_file_info *stat) {
  return file->vtable->stat(file, stat);
}

bool otter_file_set_owner(otter_file *file, const otter_file_owner *owner) {
  return file->vtable->set_owner(file, owner);
}

bool otter_file_set_permissions(otter_file *file,
                                const otter_file_permissions *permissions) {
  return file->vtable->set_permissions(file, permissions);
}

OTTER_DEFINE_TRIVIAL_CLEANUP_FUNC(otter_file *, otter_file_close);
