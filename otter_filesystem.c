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
#include "otter_filesystem.h"
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <sys/xattr.h>
typedef struct otter_file_impl {
  otter_file base;
  otter_allocator *allocator;
  FILE *handle;
} otter_file_impl;

typedef struct otter_filesystem_impl {
  otter_filesystem base;
  otter_allocator *allocator;
} otter_filesystem_impl;

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
  };

  otter_file_impl *file = otter_malloc(filesystem->allocator, sizeof(*file));
  file->base.vtable = &vtable;
  file->handle = file_handle;
  file->allocator = filesystem->allocator;
  return (otter_file *)file;
}

static int otter_filesystem_get_attribute_impl(otter_filesystem *,
                                               const char *path,
                                               const char *attribute,
                                               unsigned char *value,
                                               size_t value_size) {
  ssize_t result = getxattr(path, attribute, value, value_size);
  assert(result >= INT_MIN);
  assert(result <= INT_MAX);
  return (int)result;
}

static int otter_filesystem_set_attribute_impl(otter_filesystem *,
                                               const char *path,
                                               const char *attribute,
                                               const unsigned char *value,
                                               size_t value_size) {
  return setxattr(path, attribute, value, value_size, 0);
}

otter_filesystem *otter_filesystem_create(otter_allocator *allocator) {
  otter_filesystem_impl *fs = otter_malloc(allocator, sizeof(*fs));
  if (fs == NULL) {
    return NULL;
  }

  static otter_filesystem_vtable vtable = {
      .open_file = otter_filesystem_open_file_impl,
      .set_attribute = otter_filesystem_set_attribute_impl,
      .get_attribute = otter_filesystem_get_attribute_impl,
  };

  fs->base.vtable = &vtable;
  fs->allocator = allocator;
  return (otter_filesystem *)fs;
}

otter_file *otter_filesystem_open_file(otter_filesystem *filesystem,
                                       const char *path, const char *mode) {
  return filesystem->vtable->open_file(filesystem, path, mode);
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
