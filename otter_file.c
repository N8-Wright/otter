#include "otter_file.h"

size_t otter_file_read(otter_file *file, void *buffer, size_t num_bytes) {
  return file->vtable->read(file, buffer, num_bytes);
}

size_t otter_file_write(otter_file *file, const void *buffer,
                        size_t num_bytes) {
  return file->vtable->write(file, buffer, num_bytes);
}

void otter_file_close(otter_file *file) { file->vtable->close(file); }
