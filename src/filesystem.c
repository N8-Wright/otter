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
#include "otter/filesystem.h"
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/xattr.h>
#include <unistd.h>
typedef struct otter_file_impl {
  otter_file base;
  otter_allocator *allocator;
  FILE *handle;
} otter_file_impl;

typedef struct otter_filesystem_impl {
  otter_filesystem base;
  otter_allocator *allocator;
} otter_filesystem_impl;

static bool otter_file_set_owner_impl(otter_file *file_,
                                      const otter_file_owner *owner) {
  otter_file_impl *file = (otter_file_impl *)file_;
  const int result = fchown(fileno(file->handle), owner->uid, owner->gid);
  return result == 0;
}

static bool
otter_file_set_permissions_impl(otter_file *file_,
                                const otter_file_permissions *permissions) {
  otter_file_impl *file = (otter_file_impl *)file_;
  const int result = fchmod(fileno(file->handle), permissions->mode);
  return result == 0;
}

static bool otter_file_stat_impl(otter_file *file_, otter_file_info *stat) {
  otter_file_impl *file = (otter_file_impl *)file_;
  const int result = fstat(fileno(file->handle), &stat->value);
  return result == 0;
}

static void otter_file_close_impl(otter_file *file_) {
  otter_file_impl *file = (otter_file_impl *)file_;
  fclose(file->handle);
  otter_free(file->allocator, file);
}

static size_t otter_file_write_impl(otter_file *file_, const void *buffer,
                                    size_t num_bytes) {
  otter_file_impl *file = (otter_file_impl *)file_;
  const size_t bytes_written = fwrite(buffer, 1, num_bytes, file->handle);
  return bytes_written;
}

static size_t otter_file_read_impl(otter_file *file_, void *buffer,
                                   size_t num_bytes) {
  otter_file_impl *file = (otter_file_impl *)file_;
  const size_t bytes_read = fread(buffer, 1, num_bytes, file->handle);
  return bytes_read;
}

static void otter_filesystem_free_impl(otter_filesystem *filesystem_) {
  otter_filesystem_impl *filesystem = (otter_filesystem_impl *)filesystem_;
  otter_free(filesystem->allocator, filesystem);
}

static otter_file *
otter_filesystem_open_file_impl(otter_filesystem *filesystem_, const char *path,
                                const char *mode) {
  otter_filesystem_impl *filesystem = (otter_filesystem_impl *)filesystem_;
  FILE *file_handle = fopen(path, mode);
  if (file_handle == NULL) {
    return NULL;
  }

  static otter_file_vtable vtable = {
      .read = otter_file_read_impl,
      .write = otter_file_write_impl,
      .close = otter_file_close_impl,
      .stat = otter_file_stat_impl,
      .set_owner = otter_file_set_owner_impl,
      .set_permissions = otter_file_set_permissions_impl,
  };

  otter_file_impl *file = otter_malloc(filesystem->allocator, sizeof(*file));
  if (file == NULL) {
    fclose(file_handle);
    return NULL;
  }

  file->base.vtable = &vtable;
  file->handle = file_handle;
  file->allocator = filesystem->allocator;
  return (otter_file *)file;
}

static bool otter_filesystem_copy_impl(otter_filesystem *filesystem_,
                                       const char *from_path,
                                       const char *to_path) {
  OTTER_CLEANUP(otter_file_close_p)
  otter_file *from_file =
      otter_filesystem_open_file(filesystem_, from_path, "rb");
  if (from_file == NULL) {
    return false;
  }

  OTTER_CLEANUP(otter_file_close_p)
  otter_file *to_file = otter_filesystem_open_file(filesystem_, to_path, "wb");
  if (to_file == NULL) {
    return false;
  }

  otter_file_info stat_info;
  if (!otter_file_stat(from_file, &stat_info)) {
    return false;
  }

  otter_file_owner owner = otter_file_info_get_owner(&stat_info);
  otter_file_permissions permissions =
      otter_file_info_get_permissions(&stat_info);
  if (!otter_file_set_owner(to_file, &owner)) {
    return false;
  }

  if (!otter_file_set_permissions(to_file, &permissions)) {
    return false;
  }

#define buffer_size 4096
  char buffer[buffer_size];
  size_t bytes_read = otter_file_read(from_file, buffer, buffer_size);
  while (bytes_read > 0) {
    const size_t bytes_written = otter_file_write(to_file, buffer, bytes_read);
    if (bytes_written != bytes_read) {
      return false;
    }

    bytes_read = otter_file_read(from_file, buffer, buffer_size);
  }

  return true;
}

static bool otter_filesystem_remove_impl(otter_filesystem * /*unused*/,
                                         const char *path) {
  const int result = remove(path);
  return result == 0;
}

static bool otter_filesystem_exists_impl(otter_filesystem * /*unused*/,
                                         const char *path) {
  if (access(path, F_OK) == 0) {
    return true;
  }

  return false;
}

static int otter_filesystem_get_attribute_impl(otter_filesystem * /*unused*/,
                                               const char *path,
                                               const char *attribute,
                                               unsigned char *value,
                                               size_t value_size) {
  ssize_t result = getxattr(path, attribute, value, value_size);
  assert(result >= INT_MIN);
  assert(result <= INT_MAX);
  return (int)result;
}

static int otter_filesystem_set_attribute_impl(otter_filesystem * /*unused */,
                                               const char *path,
                                               const char *attribute,
                                               const unsigned char *value,
                                               size_t value_size) {
  return setxattr(path, attribute, value, value_size, 0);
}

otter_filesystem *otter_filesystem_create(otter_allocator *allocator) {
  otter_filesystem_impl *filesystem =
      otter_malloc(allocator, sizeof(*filesystem));
  if (filesystem == NULL) {
    return NULL;
  }

  static otter_filesystem_vtable vtable = {
      .free = otter_filesystem_free_impl,
      .open_file = otter_filesystem_open_file_impl,
      .copy = otter_filesystem_copy_impl,
      .remove = otter_filesystem_remove_impl,
      .exists = otter_filesystem_exists_impl,
      .set_attribute = otter_filesystem_set_attribute_impl,
      .get_attribute = otter_filesystem_get_attribute_impl,
  };

  filesystem->base.vtable = &vtable;
  filesystem->allocator = allocator;
  return (otter_filesystem *)filesystem;
}

void otter_filesystem_free(otter_filesystem *filesystem) {
  filesystem->vtable->free(filesystem);
}

OTTER_DEFINE_TRIVIAL_CLEANUP_FUNC(otter_filesystem *, otter_filesystem_free);

otter_file *otter_filesystem_open_file(otter_filesystem *filesystem,
                                       const char *path, const char *mode) {
  return filesystem->vtable->open_file(filesystem, path, mode);
}

bool otter_filesystem_copy(otter_filesystem *filesystem, const char *from_path,
                           const char *to_path) {
  return filesystem->vtable->copy(filesystem, from_path, to_path);
}

bool otter_filesystem_remove(otter_filesystem *filesystem, const char *path) {
  return filesystem->vtable->remove(filesystem, path);
}

bool otter_filesystem_exists(otter_filesystem *filesystem, const char *path) {
  return filesystem->vtable->exists(filesystem, path);
}

int otter_filesystem_get_attribute(otter_filesystem *filesystem,
                                   const char *path, const char *attribute,
                                   unsigned char *value, size_t value_size) {
  return filesystem->vtable->get_attribute(filesystem, path, attribute, value,
                                           value_size);
}
int otter_filesystem_set_attribute(otter_filesystem *filesystem,
                                   const char *path, const char *attribute,
                                   const unsigned char *value,
                                   size_t value_size) {
  return filesystem->vtable->set_attribute(filesystem, path, attribute, value,
                                           value_size);
}
