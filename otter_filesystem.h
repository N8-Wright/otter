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
#ifndef OTTER_FILESYSTEM_H_
#define OTTER_FILESYSTEM_H_
#include "otter_allocator.h"
#include "otter_file.h"
#include "otter_inc.h"
#include <stddef.h>
typedef struct otter_filesystem otter_filesystem;
typedef struct otter_filesystem_vtable {
  void (*free)(otter_filesystem *);
  otter_file *(*open_file)(otter_filesystem *, const char *path,
                           const char *mode);
  int (*get_attribute)(otter_filesystem *, const char *path,
                       const char *attribute, unsigned char *value,
                       size_t value_size);
  int (*set_attribute)(otter_filesystem *, const char *path,
                       const char *attribute, const unsigned char *value,
                       size_t value_size);
} otter_filesystem_vtable;

struct otter_filesystem {
  otter_filesystem_vtable *vtable;
};

otter_filesystem *otter_filesystem_create(otter_allocator *allocator);
void otter_filesystem_free(otter_filesystem *filesystem);
OTTER_DECLARE_TRIVIAL_CLEANUP_FUNC(otter_filesystem *, otter_filesystem_free);
otter_file *otter_filesystem_open_file(otter_filesystem *filesystem,
                                       const char *path, const char *mode);
int otter_filesystem_get_attribute(otter_filesystem *filesystem,
                                   const char *path, const char *attribute,
                                   unsigned char *value, size_t value_size);
int otter_filesystem_set_attribute(otter_filesystem *filesystem,
                                   const char *path, const char *attribute,
                                   const unsigned char *value,
                                   size_t value_size);

#endif /* OTTER_FILESYSTEM_H_ */
