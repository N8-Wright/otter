#ifndef OTTER_H_
#define OTTER_H_
#include "otter_allocator.h"

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
                                  ...);
void otter_target_add_command(otter_target *target, const char *command);
void otter_target_add_dependency(otter_target *target, otter_target *dep);

void otter_target_argv_insert(otter_target *target, char *arg);
bool otter_target_files_insert(otter_target *target, char *arg);

bool otter_target_generate_hash(otter_target *target, unsigned char *hash,
                                unsigned int *hash_size);
bool otter_target_needs_execute(otter_target *target);
void otter_target_store_hash(otter_target *target);
#endif /* OTTER_H_ */
