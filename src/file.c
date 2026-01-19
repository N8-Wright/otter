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

size_t otter_file_read(otter_file *file, void *buffer, size_t num_bytes) {
  return file->vtable->read(file, buffer, num_bytes);
}

size_t otter_file_write(otter_file *file, const void *buffer,
                        size_t num_bytes) {
  return file->vtable->write(file, buffer, num_bytes);
}

void otter_file_close(otter_file *file) { file->vtable->close(file); }
OTTER_DEFINE_TRIVIAL_CLEANUP_FUNC(otter_file *, otter_file_close);
