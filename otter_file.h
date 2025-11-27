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
