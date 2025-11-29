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
#include "otter_target.h"
#include "otter_cstring.h"

#include <assert.h>
#include <errno.h>
#include <openssl/evp.h>
#include <spawn.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/xattr.h>

extern char **environ;

int otter_target_execute_dependency(otter_target *target, bool *executed) {
  if (executed == NULL) {
    return -1;
  }

  if (target == NULL) {
    *executed = false;
    return -1;
  }

  bool any_dependency_executed = false;
  otter_dependency *dependency = target->dependencies;
  while (dependency != NULL) {
    bool dependency_executed = false;
    int result = otter_target_execute_dependency(dependency->target,
                                                 &dependency_executed);
    if (dependency_executed) {
      any_dependency_executed = true;
    }

    if (result != 0) {
      return result;
    }

    dependency = dependency->next;
  }

  if (any_dependency_executed || otter_target_needs_execute(target)) {
    *executed = true;
    printf("Executing target '%s'...\n", target->name);
    printf("    Command: '");
    printf("%s", target->argv[0]);
    for (size_t i = 1; i < target->argv_length; i++) {
      printf(" %s", target->argv[i]);
    }
    printf("'\n");

    otter_target_argv_insert(target, NULL);
    pid_t pid;
    int posix_spawn_result =
        posix_spawnp(&pid, target->argv[0], NULL, NULL, target->argv, environ);
    if (posix_spawn_result == 0) {
      int status;
      if (waitpid(pid, &status, 0) > 0) {
        if (!WIFEXITED(status)) {
          fprintf(stderr, "Failed in execution of command %s\n",
                  target->argv[0]);
        }

        otter_target_store_hash(target);
        printf("    Finished\n");

        return status;
      }

      return -1;
    } else {
      fprintf(stderr, "Failed to spawn target process %s beacuse '%s'\n",
              target->argv[0], strerror(posix_spawn_result));
    }

    return -1;
  } else {
    *executed = false;
    printf("Target '%s' up-to-date...\n", target->name);
    return 0;
  }
}

int otter_target_execute(otter_target *target) {
  bool executed = false;
  return otter_target_execute_dependency(target, &executed);
}

void otter_target_free(otter_target *target) {
  otter_free(target->allocator, target->name);

  for (size_t i = 0; i < target->files_length; i++) {
    otter_free(target->allocator, target->files[i]);
  }

  otter_free(target->allocator, target->files);

  for (size_t i = 0; i < target->argv_length; i++) {
    otter_free(target->allocator, target->argv[i]);
  }

  otter_free(target->allocator, target->argv);

  otter_dependency *dep = target->dependencies;
  while (dep != NULL) {
    otter_dependency *tmp = dep;
    dep = dep->next;
    otter_free(target->allocator, tmp);
  }

  otter_free(target->allocator, target->hash);
  otter_free(target->allocator, target);
}

otter_target *otter_target_create(const char *name, otter_allocator *allocator,
                                  otter_filesystem *filesystem,
                                  otter_logger *logger, ...) {
  otter_target *target = otter_malloc(allocator, sizeof(*target));
  if (target == NULL) {
    return NULL;
  }

  target->allocator = allocator;
  target->filesystem = filesystem;
  target->logger = logger;
  target->name = otter_strdup(allocator, name);
  if (target->name == NULL) {
    otter_log_critical(target->logger, "Failed to allocate target's name");
    otter_free(allocator, target);
    return NULL;
  }

  target->hash = NULL;
  target->hash_size = 0;

  target->dependencies = NULL;

  target->files_length = 0;
  target->files_capacity = 6;
  target->files =
      otter_malloc(allocator, sizeof(char *) * target->files_capacity);
  if (target->files == NULL) {
    otter_log_critical(target->logger, "Failed to allocate target files array");
    otter_free(allocator, target->name);
    otter_free(allocator, target);
  }

  target->argv_length = 0;
  target->argv_capacity = 6;
  target->argv =
      otter_malloc(allocator, sizeof(char *) * target->argv_capacity);
  if (target->argv == NULL) {
    otter_log_critical(target->logger,
                       "Failed to allocate target argumets array");
    otter_free(allocator, target->name);
    otter_free(allocator, target->files);
    otter_free(allocator, target);
    return NULL;
  }

  va_list args;
  va_start(args, logger);
  const char *file = va_arg(args, const char *);
  while (file != NULL) {
    char *duplicated_file = otter_strdup(allocator, file);
    if (duplicated_file == NULL) {
      otter_log_critical(target->logger, "Failed to duplicate file name: '%s'",
                         file);
      otter_free(allocator, target->name);
      for (size_t i = 0; i < target->files_length; i++) {
        otter_free(allocator, target->files[i]);
      }

      otter_free(allocator, target->files);
      otter_free(allocator, target->argv);
      otter_free(allocator, target);
      return NULL;
    }

    if (!otter_target_files_insert(target, duplicated_file)) {
      otter_log_critical(target->logger,
                         "Failed to insert duplicated file into array: '%s'",
                         duplicated_file);
      otter_free(allocator, duplicated_file);
      otter_free(allocator, target->name);
      for (size_t i = 0; i < target->files_length; i++) {
        otter_free(allocator, target->files[i]);
      }

      otter_free(allocator, target->files);
      otter_free(allocator, target->argv);
      otter_free(allocator, target);
      return NULL;
    }

    file = va_arg(args, const char *);
  }

  va_end(args);
  if (!otter_target_generate_hash(target)) {
    otter_target_free(target);
    return NULL;
  }

  return target;
}

void otter_target_add_command(otter_target *target, const char *command_) {
  const char *delims = " \t\n";
  char *command = otter_strdup(target->allocator, command_);
  if (command == NULL) {
    return;
  }

  char *token = strtok(command, delims);
  while (token != NULL) {
    char *arg = otter_strdup(target->allocator, token);
    if (arg == NULL) {
      otter_free(target->allocator, command);
      return;
    }

    otter_target_argv_insert(target, arg);
    token = strtok(NULL, delims);
  }

  otter_free(target->allocator, command);
}

void otter_target_add_dependency(otter_target *target, otter_target *dep) {
  otter_dependency *wrapper = otter_malloc(target->allocator, sizeof(*wrapper));
  if (wrapper == NULL) {
    return;
  }

  wrapper->target = dep;
  wrapper->next = target->dependencies;
  target->dependencies = wrapper;
}

void otter_target_argv_insert(otter_target *target, char *arg) {
  if (target->argv_length >= target->argv_capacity) {
    target->argv_capacity *= 2;
    void *result = otter_realloc(target->allocator, target->argv,
                                 sizeof(char *) * target->argv_capacity);
    if (result == NULL) {
      return;
    }

    target->argv = result;
  }

  target->argv[target->argv_length++] = arg;
}

bool otter_target_files_insert(otter_target *target, char *arg) {
  if (target->files_length >= target->files_capacity) {
    target->files_capacity *= 2;
    void *result = otter_realloc(target->allocator, target->files,
                                 sizeof(char *) * target->files_capacity);
    if (result == NULL) {
      return false;
    }

    target->files = result;
  }

  target->files[target->files_length++] = arg;
  return true;
}

bool otter_target_generate_hash(otter_target *target) {
  EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
  if (mdctx == NULL) {
    otter_log_critical(target->logger,
                       "Unable to create new EVP_MD_CTX for file '%s'",
                       target->name);
    return false;
  }

  if (EVP_DigestInit_ex(mdctx, EVP_sha1(), NULL) != 1) {
    otter_log_critical(target->logger,
                       "Unable to initialize SHA1 digest for file '%s'",
                       target->name);
    EVP_MD_CTX_free(mdctx);
    return false;
  }

  for (size_t i = 0; i < target->files_length; i++) {
    otter_file *file =
        otter_filesystem_open_file(target->filesystem, target->files[i], "rb");
    if (file == NULL) {
      otter_log_error(target->logger, "Unable to open file '%s': '%s'",
                      target->name, strerror(errno));
      // Target output has not been created
      return false;
    }

    unsigned char buffer[4096] = {};
    size_t bytes = 0;
    while ((bytes = otter_file_read(file, buffer, sizeof(buffer))) > 0) {
      if (EVP_DigestUpdate(mdctx, buffer, bytes) != 1) {
        otter_log_error(target->logger, "Unable to update digest for file '%s'",
                        target->name);
        otter_file_close(file);
        EVP_MD_CTX_free(mdctx);
        return false;
      }
    }

    otter_file_close(file);
  }

  target->hash = otter_malloc(target->allocator, EVP_MAX_MD_SIZE);
  if (target->hash == NULL) {
    otter_log_critical(
        target->logger,
        "Unable to allocate buffer to store digest info for file '%s'",
        target->name);
    EVP_MD_CTX_free(mdctx);
    return false;
  }

  if (EVP_DigestFinal_ex(mdctx, target->hash, &target->hash_size) != 1) {
    otter_log_error(target->logger,
                    "Unable to generate digest info for file '%s'",
                    target->name);
    EVP_MD_CTX_free(mdctx);
    return false;
  }

  EVP_MD_CTX_free(mdctx);
  return true;
}

bool otter_target_needs_execute(otter_target *target) {
  // Retrieve stored digest
  unsigned char stored_hash[EVP_MAX_MD_SIZE];
  int stored_hash_size = otter_filesystem_get_attribute(
      target->filesystem, target->name, OTTER_XATTR_NAME, stored_hash,
      sizeof(stored_hash));
  if (stored_hash_size < 0) {
    return true;
  }

  if ((unsigned int)stored_hash_size != target->hash_size) {
    return true;
  }

  if (memcmp(target->hash, stored_hash, target->hash_size) == 0) {
    return false;
  } else {
    return true;
  }
}

void otter_target_store_hash(otter_target *target) {
  // Store raw digest in xattr
  assert(target->hash != NULL);
  assert(target->hash_size != 0);
  if (otter_filesystem_set_attribute(target->filesystem, target->name,
                                     OTTER_XATTR_NAME, target->hash,
                                     target->hash_size) < 0) {
    fprintf(stderr, "Failed to set %s attribute on file '%s': '%s'\n",
            OTTER_XATTR_NAME, target->name, strerror(errno));

    return;
  }
}
