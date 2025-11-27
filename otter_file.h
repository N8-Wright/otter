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
#include <stddef.h>

typedef struct otter_file otter_file;
typedef struct otter_file_vtable {
  size_t (*read)(otter_file *, void *buffer, size_t num_bytes);
  size_t (*write)(otter_file *, const void *buffer, size_t num_bytes);
  void (*close)(otter_file *);
} otter_file_vtable;

struct otter_file {
  otter_file_vtable *vtable;
};

size_t otter_file_read(otter_file *, void *buffer, size_t num_bytes);
size_t otter_file_write(otter_file *, const void *buffer, size_t num_bytes);
void otter_file_close(otter_file *);

#endif /* OTTER_FILE_H_ */
