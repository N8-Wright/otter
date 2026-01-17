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
#include <gnutls/crypto.h>
#include <gnutls/gnutls.h>
#include <spawn.h>
#include <stdarg.h>
#include <stddef.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/xattr.h>
#include <unistd.h>

extern char **environ;
static int otter_target_execute_dependency(otter_target *target);
static bool otter_target_generate_hash_c(otter_target *target);
static int otter_target_run_clang_tidy(otter_target *target);

static void otter_target_was_executed(bool *executed, otter_target *target) {
  if (executed == NULL) {
    return;
  }

  if (target->executed) {
    otter_log_debug(target->logger, "'%s' was executed", target->name);
    *executed = true;
  } else {
    otter_log_debug(target->logger, "'%s' was not executed", target->name);
  }

  OTTER_ARRAY_FOREACH(target, dependencies, otter_target_was_executed,
                      executed);
}

static bool otter_target_needs_execute(otter_target *target) {
  otter_log_debug(target->logger, "Checking if '%s' needs to be executed",
                  target->name);
  bool any_dependency_executed = false;
  OTTER_ARRAY_FOREACH(target, dependencies, otter_target_was_executed,
                      &any_dependency_executed);
  if (any_dependency_executed) {
    if (target->type != OTTER_TARGET_OBJECT) {
      otter_log_debug(target->logger,
                      "'%s' target needs to execute because one or more of its "
                      "dependencies was exectued",
                      target->name);
      return true;
    } else {
      otter_log_debug(target->logger,
                      "One or more of %s's dependencies was executed.  This "
                      "not enough to say that '%s' needs to execute though",
                      target->name, target->name);
    }
  }

  /* Retrieve stored digest */
  unsigned int expected_hash_size = gnutls_hash_get_len(GNUTLS_DIG_SHA1);
  unsigned char *stored_hash =
      otter_malloc(target->allocator, expected_hash_size);
  if (stored_hash == NULL) {
    otter_log_critical(target->logger,
                       "Unable to allocate space for sha1 digest");
    return true;
  }

  int stored_hash_size = otter_filesystem_get_attribute(
      target->filesystem, target->name, OTTER_XATTR_NAME, stored_hash,
      expected_hash_size);
  if (stored_hash_size < 0) {
    otter_free(target->allocator, stored_hash);
    return true;
  }

  if ((unsigned int)stored_hash_size != target->hash_size) {
    otter_log_debug(
        target->logger,
        "Hashes do not match for target '%s'.  It needs to be executed.",
        target->name);
    otter_free(target->allocator, stored_hash);
    return true;
  }

  if (memcmp(target->hash, stored_hash, target->hash_size) == 0) {
    otter_log_debug(
        target->logger,
        "Hashes match for target '%s'.  It does not need to be executed.",
        target->name);
    otter_free(target->allocator, stored_hash);
    return false;
  } else {
    otter_log_debug(
        target->logger,
        "Hashes do not match for target '%s'.  It needs to be executed.",
        target->name);
    otter_free(target->allocator, stored_hash);
    return true;
  }
}

static void otter_target_store_hash(otter_target *target) {
  // Store raw digest in xattr
  assert(target->hash != NULL);
  assert(target->hash_size != 0);
  if (otter_filesystem_set_attribute(target->filesystem, target->name,
                                     OTTER_XATTR_NAME, target->hash,
                                     target->hash_size) < 0) {
    otter_log_error(target->logger,
                    "Failed to set %s attribute on file '%s': '%s'\n",
                    OTTER_XATTR_NAME, target->name, strerror(errno));

    return;
  }
}

static int otter_target_run_clang_tidy(otter_target *target) {
  if (target == NULL) {
    return -1;
  }

  /* Only run clang-tidy on targets that have source files */
  if (OTTER_ARRAY_LENGTH(target, files) == 0) {
    return 0;
  }

  int result = -1;
  char **argv = NULL;
  size_t arg_count = 0;

  /* Build argv: clang-tidy <files...> -- NULL */
  const size_t file_count = OTTER_ARRAY_LENGTH(target, files);
  const size_t argc =
      1 + file_count + 1 + 1; /* clang-tidy + files + "--" + NULL */
  argv = otter_malloc(target->allocator, argc * sizeof(char *));
  if (argv == NULL) {
    otter_log_error(target->logger, "Failed to allocate argv for clang-tidy");
    goto cleanup;
  }

  /* Initialize all slots to NULL for safe cleanup */
  for (size_t i = 0; i < argc; i++) {
    argv[i] = NULL;
  }

  argv[arg_count] = otter_strdup(target->allocator, "clang-tidy");
  if (argv[arg_count] == NULL) {
    goto cleanup;
  }
  arg_count++;

  for (size_t i = 0; i < file_count; i++) {
    argv[arg_count] = otter_strdup(target->allocator,
                                   OTTER_ARRAY_AT_UNSAFE(target, files, i));
    if (argv[arg_count] == NULL) {
      goto cleanup;
    }
    arg_count++;
  }

  argv[arg_count] = otter_strdup(target->allocator, "--");
  if (argv[arg_count] == NULL) {
    goto cleanup;
  }
  arg_count++;

  otter_log_info(target->logger, "Running clang-tidy on target '%s'",
                 target->name);

  pid_t pid;
  const int posix_spawn_result =
      posix_spawnp(&pid, "clang-tidy", NULL, NULL, argv, environ);

  if (posix_spawn_result != 0) {
    otter_log_error(target->logger,
                    "Failed to spawn clang-tidy for target '%s': '%s'",
                    target->name, strerror(posix_spawn_result));
    goto cleanup;
  }

  int status;
  if (waitpid(pid, &status, 0) > 0) {
    if (!WIFEXITED(status)) {
      otter_log_error(target->logger, "clang-tidy failed for target '%s'",
                      target->name);
      goto cleanup;
    }
    result = WEXITSTATUS(status);
  }

cleanup:
  if (argv != NULL) {
    for (size_t i = 0; i < argc; i++) {
      otter_free(target->allocator, argv[i]);
    }
    otter_free(target->allocator, argv);
  }

  return result;
}

static void otter_target_execute_dependency_helper(int *return_code,
                                                   otter_target *dependency) {

  int result = otter_target_execute_dependency(dependency);
  *return_code += result;
  if (result != 0) {
    return;
  }
}

static int otter_target_execute_dependency(otter_target *target) {
  if (target == NULL) {
    return -1;
  }
  otter_log_debug(target->logger, "%s: attempting to execute '%s'", __func__,
                  target->name);
  int return_code = 0;
  OTTER_ARRAY_FOREACH(target, dependencies,
                      otter_target_execute_dependency_helper, &return_code);
  if (return_code != 0) {
    return return_code;
  }

  if (otter_target_needs_execute(target)) {
    if (!OTTER_ARRAY_APPEND(target, argv, target->allocator, NULL)) {
      return -1;
    }

    int clang_tidy_result = otter_target_run_clang_tidy(target);
    if (clang_tidy_result != 0) {
      otter_log_error(target->logger, "clang-tidy failed for target '%s'",
                      target->name);
      return clang_tidy_result;
    }

    target->executed = true;
    otter_log_info(target->logger, "Executing target '%s'\nCommand: '%s'",
                   target->name, target->command);
    pid_t pid;
    int posix_spawn_result =
        posix_spawnp(&pid, target->argv[0], NULL, NULL, target->argv, environ);
    if (posix_spawn_result == 0) {
      int status;
      if (waitpid(pid, &status, 0) > 0) {
        if (!WIFEXITED(status)) {
          otter_log_error(target->logger, "Failed in execution of command %s\n",
                          target->argv[0]);
        } else {
          if (WEXITSTATUS(status) == 0) {
            otter_target_store_hash(
                target); /* Only update hash on success.  Allows for the target
                            to be re-executed. */
          }
        }

        return status;
      }

      return -1;
    } else {
      otter_log_error(target->logger,
                      "Failed to spawn target process %s beacuse '%s'\n",
                      target->argv[0], strerror(posix_spawn_result));
    }

    return -1;
  } else {
    return 0;
  }
}

int otter_target_execute(otter_target *target) {
  if (target == NULL) {
    return -1;
  }

  int return_code = 0;
  OTTER_ARRAY_FOREACH(target, dependencies,
                      otter_target_execute_dependency_helper, &return_code);
  if (return_code != 0) {
    return return_code;
  }

  if (otter_target_needs_execute(target)) {
    if (!OTTER_ARRAY_APPEND(target, argv, target->allocator, NULL)) {
      return -1;
    }

    int clang_tidy_result = otter_target_run_clang_tidy(target);
    if (clang_tidy_result != 0) {
      otter_log_error(target->logger, "clang-tidy failed for target '%s'",
                      target->name);
      return clang_tidy_result;
    }

    target->executed = true;
    otter_log_info(target->logger, "Executing target '%s'\nCommand: '%s'",
                   target->name, target->command);
    pid_t pid;
    int posix_spawn_result =
        posix_spawnp(&pid, target->argv[0], NULL, NULL, target->argv, environ);
    if (posix_spawn_result == 0) {
      int status;
      if (waitpid(pid, &status, 0) > 0) {
        if (!WIFEXITED(status)) {
          otter_log_error(target->logger, "Failed in execution of command %s\n",
                          target->argv[0]);
        } else {
          if (WEXITSTATUS(status) == 0) {
            otter_target_store_hash(
                target); /* Only update hash on success.  Allows for the target
                            to be re-executed. */
          }
        }
        return status;
      }

      return -1;
    } else {
      otter_log_error(target->logger,
                      "Failed to spawn target process %s beacuse '%s'\n",
                      target->argv[0], strerror(posix_spawn_result));
    }

    return -1;
  } else {
    otter_log_info(target->logger, "Target '%s' up-to-date", target->name);
    return 0;
  }
}

void otter_target_free(otter_target *target) {
  otter_free(target->allocator, target->name);
  OTTER_ARRAY_FOREACH(target, files, otter_free, target->allocator);
  otter_free(target->allocator, target->files);
  otter_free(target->allocator, target->command);
  OTTER_ARRAY_FOREACH(target, argv, otter_free, target->allocator);
  otter_free(target->allocator, target->argv);
  otter_free(target->allocator, target->dependencies);
  otter_free(target->allocator, target->hash);
  otter_free(target->allocator, target);
}

OTTER_DEFINE_TRIVIAL_CLEANUP_FUNC(otter_target *, otter_target_free);

static bool otter_target_generate_command_from_argv(otter_target *target) {
  size_t command_length = 0;
  for (size_t i = 0; i < OTTER_ARRAY_LENGTH(target, argv); i++) {
    const char *arg = OTTER_ARRAY_AT_UNSAFE(target, argv, i);
    command_length += strlen(arg) + 1; // Additional byte for ' '
  }

  /* NOTE: Additional byte for '\0' was already accounted for in the
     last iteration of the for loop above */
  target->command = otter_malloc(target->allocator, command_length);
  if (target->command == NULL) {
    otter_log_critical(target->logger,
                       "Unable to allocate string '%s' of size %zd",
                       OTTER_NAMEOF(target->command), command_length + 1);
    return false;
  }

  size_t offset = 0;
  size_t i = 0;
  for (; i < OTTER_ARRAY_LENGTH(target, argv) - 1; i++) {
    const char *arg = OTTER_ARRAY_AT_UNSAFE(target, argv, i);
    size_t arg_length = strlen(arg);
    memcpy(target->command + offset, arg, arg_length);
    offset += arg_length;
    target->command[offset] = ' ';
    offset += 1;
  }

  const char *arg = OTTER_ARRAY_AT(target, argv, i);
  size_t arg_length = strlen(arg);
  memcpy(target->command + offset, arg, arg_length);
  offset += arg_length;

  assert(offset == command_length - 1);
  target->command[offset] = '\0';
  return true;
}

static bool otter_target_append_arg_to_argv(otter_target *target,
                                            const char *arg_) {
  if (target == NULL || arg_ == NULL) {
    return false;
  }

  for (size_t i = 0; i < OTTER_ARRAY_LENGTH(target, argv); i++) {
    const char *existing_arg = OTTER_ARRAY_AT_UNSAFE(target, argv, i);
    if (0 == strcmp(existing_arg, arg_)) {
      otter_log_debug(
          target->logger,
          "Skipping adding argument '%s' to argv since it already exists",
          existing_arg);
      return true;
    }
  }

  char *arg = otter_strdup(target->allocator, arg_);
  if (arg == NULL) {
    otter_log_critical(target->logger, "Failed to duplicate argument '%s'",
                       arg_);
    return false;
  }

  if (!OTTER_ARRAY_APPEND(target, argv, target->allocator, arg)) {
    otter_log_critical(target->logger,
                       "Failed to append argument '%s' to array %s", arg,
                       OTTER_NAMEOF(target->argv));
    otter_free(target->allocator, arg);
    return false;
  }

  return true;
}

static bool otter_target_append_args_to_argv(otter_target *target,
                                             const char *args_) {
  const char *delims = " \t\n";
  char *args = otter_strdup(target->allocator, args_);
  if (args == NULL) {
    otter_log_critical(target->logger, "Failed to duplicate arguments '%s'",
                       args_);
    return false;
  }

  char *token = strtok(args, delims);
  while (token != NULL) {
    if (!otter_target_append_arg_to_argv(target, token)) {
      otter_free(target->allocator, args);
      return false;
    }

    token = strtok(NULL, delims);
  }

  otter_free(target->allocator, args);
  return true;
}

static bool otter_target_generate_c_object_argv(otter_target *target,
                                                const char *cc_flags) {
  if (target == NULL || cc_flags == NULL) {
    return false;
  }

  if (!otter_target_append_args_to_argv(target, "cc -fPIC -c")) {
    return false;
  }

  for (size_t i = 0; i < OTTER_ARRAY_LENGTH(target, files); i++) {
    if (!otter_target_append_arg_to_argv(
            target, OTTER_ARRAY_AT_UNSAFE(target, files, i))) {
      return false;
    }
  }

  if (!otter_target_append_arg_to_argv(target, "-o")) {
    return false;
  }

  if (!otter_target_append_arg_to_argv(target, target->name)) {
    return false;
  }

  if (!otter_target_append_args_to_argv(target, cc_flags)) {
    return false;
  }

  return otter_target_generate_command_from_argv(target);
}

static bool otter_target_append_objects_to_argv(otter_target *target,
                                                otter_target *dependency) {
  if (dependency->type == OTTER_TARGET_OBJECT) {
    if (!otter_target_append_arg_to_argv(target, dependency->name)) {
      return false;
    }
  }

  for (size_t i = 0; i < OTTER_ARRAY_LENGTH(dependency, dependencies); i++) {
    if (!otter_target_append_objects_to_argv(
            target, OTTER_ARRAY_AT_UNSAFE(dependency, dependencies, i))) {
      return false;
    }
  }

  return true;
}

static bool otter_target_generate_c_executable_argv(otter_target *target,
                                                    const char *cc_flags) {
  if (target == NULL || cc_flags == NULL) {
    return false;
  }

  if (!otter_target_append_args_to_argv(target, "cc -o")) {
    return false;
  }

  if (!otter_target_append_arg_to_argv(target, target->name)) {
    return false;
  }

  for (size_t i = 0; i < OTTER_ARRAY_LENGTH(target, files); i++) {
    if (!otter_target_append_arg_to_argv(
            target, OTTER_ARRAY_AT_UNSAFE(target, files, i))) {
      return false;
    }
  }

  for (size_t i = 0; i < OTTER_ARRAY_LENGTH(target, dependencies); i++) {
    if (!otter_target_append_objects_to_argv(
            target, OTTER_ARRAY_AT_UNSAFE(target, dependencies, i))) {
      return false;
    }
  }

  if (!otter_target_append_args_to_argv(target, cc_flags)) {
    return false;
  }

  return otter_target_generate_command_from_argv(target);
}

static bool otter_target_generate_c_shared_object_argv(otter_target *target,
                                                       const char *cc_flags) {
  if (target == NULL || cc_flags == NULL) {
    return false;
  }

  if (!otter_target_append_args_to_argv(target, "cc -shared -fPIC -o")) {
    return false;
  }

  if (!otter_target_append_arg_to_argv(target, target->name)) {
    return false;
  }

  for (size_t i = 0; i < OTTER_ARRAY_LENGTH(target, files); i++) {
    if (!otter_target_append_arg_to_argv(
            target, OTTER_ARRAY_AT_UNSAFE(target, files, i))) {
      return false;
    }
  }

  for (size_t i = 0; i < OTTER_ARRAY_LENGTH(target, dependencies); i++) {
    if (!otter_target_append_objects_to_argv(
            target, OTTER_ARRAY_AT_UNSAFE(target, dependencies, i))) {
      return false;
    }
  }

  if (!otter_target_append_args_to_argv(target, cc_flags)) {
    return false;
  }

  return otter_target_generate_command_from_argv(target);
}

static otter_target *otter_target_create_and_initialize(
    const char *name, otter_allocator *allocator, otter_filesystem *filesystem,
    otter_logger *logger, otter_target_type type) {
  OTTER_RETURN_IF_NULL(logger, name, NULL);
  OTTER_RETURN_IF_NULL(logger, allocator, NULL);
  OTTER_RETURN_IF_NULL(logger, filesystem, NULL);
  OTTER_RETURN_IF_NULL(logger, logger, NULL);
  otter_target *target = otter_malloc(allocator, sizeof(*target));
  if (target == NULL) {
    otter_log_critical(logger, "Unable to allocate %zd bytes for %s",
                       sizeof(*target), OTTER_NAMEOF(target));
    return NULL;
  }

  target->allocator = allocator;
  target->filesystem = filesystem;
  target->logger = logger;
  target->name = NULL;
  target->files = NULL;
  target->command = NULL;
  target->argv = NULL;
  target->dependencies = NULL;
  target->hash = NULL;
  target->hash_size = 0;
  target->executed = false;
  target->type = type;

  target->name = otter_strdup(allocator, name);
  if (target->name == NULL) {
    otter_log_critical(target->logger, "Failed to strdup %s",
                       OTTER_NAMEOF(target->name));
    goto failure;
  }

  OTTER_ARRAY_INIT(target, dependencies, target->allocator);
  if (target->dependencies == NULL) {
    otter_log_critical(target->logger, "Failed to allocate array of %s",
                       OTTER_NAMEOF(target->dependencies));
    goto failure;
  }

  OTTER_ARRAY_INIT(target, files, target->allocator);
  if (target->files == NULL) {
    otter_log_critical(target->logger, "Failed to allocate array of %s",
                       OTTER_NAMEOF(target->files));
    goto failure;
  }

  OTTER_ARRAY_INIT(target, argv, target->allocator);
  if (target->argv == NULL) {
    otter_log_critical(target->logger, "Failed to allocate array of %s",
                       OTTER_NAMEOF(target->argv));
    goto failure;
  }

  return target;
failure:
  otter_target_free(target);
  return NULL;
}

otter_target *otter_target_create_c_object(const char *name,
                                           const char *cc_flags,
                                           otter_allocator *allocator,
                                           otter_filesystem *filesystem,
                                           otter_logger *logger, ...) {
  OTTER_RETURN_IF_NULL(logger, name, NULL);
  OTTER_RETURN_IF_NULL(logger, allocator, NULL);
  OTTER_RETURN_IF_NULL(logger, filesystem, NULL);
  OTTER_RETURN_IF_NULL(logger, logger, NULL);

  otter_target *target = otter_target_create_and_initialize(
      name, allocator, filesystem, logger, OTTER_TARGET_OBJECT);
  if (target == NULL) {
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
      goto failure;
    }

    if (!OTTER_ARRAY_APPEND(target, files, target->allocator,
                            duplicated_file)) {
      otter_log_critical(target->logger,
                         "Failed to insert duplicated file '%s' into %s",
                         duplicated_file, OTTER_NAMEOF(target->files));
      goto failure;
    }

    file = va_arg(args, const char *);
  }

  va_end(args);
  otter_target_generate_c_object_argv(target, cc_flags);
  if (!otter_target_generate_hash_c(target)) {
    goto failure;
  }

  return target;
failure:
  otter_target_free(target);
  return NULL;
}

otter_target *otter_target_create_c_executable(
    const char *name, const char *flags, otter_allocator *allocator,
    otter_filesystem *filesystem, otter_logger *logger, const char **files,
    otter_target **dependencies) {
  OTTER_RETURN_IF_NULL(logger, name, NULL);
  OTTER_RETURN_IF_NULL(logger, allocator, NULL);
  OTTER_RETURN_IF_NULL(logger, filesystem, NULL);
  OTTER_RETURN_IF_NULL(logger, logger, NULL);
  OTTER_RETURN_IF_NULL(logger, files, NULL);

  otter_target *target = otter_target_create_and_initialize(
      name, allocator, filesystem, logger, OTTER_TARGET_EXECUTABLE);
  if (target == NULL) {
    return NULL;
  }

  const char **file = files;
  while (*file != NULL) {
    char *duplicated_file = otter_strdup(target->allocator, *file);
    if (duplicated_file == NULL) {
      otter_log_critical(target->logger, "Unable to duplicate string '%s'",
                         *file);
      goto failure;
    }

    if (!OTTER_ARRAY_APPEND(target, files, target->allocator,
                            duplicated_file)) {
      goto failure;
    }

    file++;
  }

  otter_target **dependency = dependencies;
  while (*dependency != NULL) {
    if (!OTTER_ARRAY_APPEND(target, dependencies, target->allocator,
                            *dependency)) {
      otter_log_critical(target->logger, "Failed to append target to %s",
                         OTTER_NAMEOF(target->dependencies));
      goto failure;
    }

    dependency++;
  }

  otter_target_generate_c_executable_argv(target, flags);
  if (!otter_target_generate_hash_c(target)) {
    goto failure;
  }

  return target;

failure:
  otter_target_free(target);
  return NULL;
}

otter_target *otter_target_create_c_shared_object(
    const char *name, const char *flags, otter_allocator *allocator,
    otter_filesystem *filesystem, otter_logger *logger, const char **files,
    otter_target **dependencies) {
  OTTER_RETURN_IF_NULL(logger, name, NULL);
  OTTER_RETURN_IF_NULL(logger, allocator, NULL);
  OTTER_RETURN_IF_NULL(logger, filesystem, NULL);
  OTTER_RETURN_IF_NULL(logger, logger, NULL);
  OTTER_RETURN_IF_NULL(logger, files, NULL);
  OTTER_RETURN_IF_NULL(logger, dependencies, NULL);

  otter_target *target = otter_target_create_and_initialize(
      name, allocator, filesystem, logger, OTTER_TARGET_SHARED_OBJECT);
  if (target == NULL) {
    return NULL;
  }

  const char **file = files;
  while (*file != NULL) {
    char *duplicated_file = otter_strdup(target->allocator, *file);
    if (duplicated_file == NULL) {
      otter_log_critical(target->logger, "Unable to duplicate string '%s'",
                         *file);
      goto failure;
    }

    if (!OTTER_ARRAY_APPEND(target, files, target->allocator,
                            duplicated_file)) {
      goto failure;
    }

    file++;
  }

  otter_target **dependency = dependencies;
  while (*dependency != NULL) {
    if (!OTTER_ARRAY_APPEND(target, dependencies, target->allocator,
                            *dependency)) {
      otter_log_critical(target->logger, "Failed to append target to %s",
                         OTTER_NAMEOF(target->dependencies));
      goto failure;
    }

    dependency++;
  }

  otter_target_generate_c_shared_object_argv(target, flags);
  if (!otter_target_generate_hash_c(target)) {
    goto failure;
  }

  return target;

failure:
  otter_target_free(target);
  return NULL;
}

void otter_target_add_command(otter_target *target, const char *command_) {
  if (target == NULL) {
    return;
  }

  if (command_ == NULL) {
    return;
  }

  target->command = otter_strdup(target->allocator, command_);
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

    if (!OTTER_ARRAY_APPEND(target, argv, target->allocator, arg)) {
      otter_free(target->allocator, command);
      otter_free(target->allocator, arg);
      OTTER_ARRAY_FOREACH(target, argv, otter_free, target->allocator);
      return;
    }

    token = strtok(NULL, delims);
  }

  otter_free(target->allocator, command);
}

void otter_target_add_dependency(otter_target *target, otter_target *dep) {
  OTTER_ARRAY_APPEND(target, dependencies, target->allocator, dep);
}

static bool otter_preprocess_and_hash_file_posix_spawnp(
    otter_target *target, gnutls_hash_hd_t hash_hd, const char *src_path) {
  int pipefd[2];
  if (pipe(pipefd) == -1) {
    otter_log_error(target->logger,
                    "Unable to create pipe for preprocessing '%s': '%s'",
                    src_path, strerror(errno));
    return false;
  }

  posix_spawn_file_actions_t actions;
  if (posix_spawn_file_actions_init(&actions) != 0) {
    otter_log_error(target->logger,
                    "posix_spawn_file_actions_init failed for '%s': '%s'",
                    src_path, strerror(errno));
    close(pipefd[0]);
    close(pipefd[1]);
    return false;
  }

  /* In the child: close read end, dup write end to stdout, and close the
   * original write fd. */
  if (posix_spawn_file_actions_addclose(&actions, pipefd[0]) != 0 ||
      posix_spawn_file_actions_adddup2(&actions, pipefd[1], STDOUT_FILENO) !=
          0 ||
      posix_spawn_file_actions_addclose(&actions, pipefd[1]) != 0) {
    otter_log_error(target->logger,
                    "posix_spawn_file_actions setup failed for '%s': '%s'",
                    src_path, strerror(errno));
    posix_spawn_file_actions_destroy(&actions);
    close(pipefd[0]);
    close(pipefd[1]);
    return false;
  }

  char *path = otter_strdup(target->allocator, src_path);
  if (path == NULL) {
    return false;
  }

  char *const argv[] = {"cc", "-E", "-P", path, NULL};
  pid_t pid;
  int spawn_err = posix_spawnp(&pid, "cc", &actions, NULL, argv, environ);

  /* actions may be destroyed regardless of spawn success */
  posix_spawn_file_actions_destroy(&actions);
  otter_free(target->allocator, path);
  if (spawn_err != 0) {
    otter_log_error(target->logger,
                    "posix_spawnp failed to run gcc for '%s': '%s'", src_path,
                    strerror(spawn_err));
    close(pipefd[0]);
    close(pipefd[1]);
    return false;
  }

  /* Parent: close write end and read from read end */
  close(pipefd[1]);
  int read_fd = pipefd[0];

  unsigned char buffer[4096];
  ssize_t n;
  while ((n = read(read_fd, buffer, sizeof(buffer))) > 0) {
    if (gnutls_hash(hash_hd, buffer, (size_t)n) < 0) {
      otter_log_error(target->logger,
                      "Unable to update hash from preprocessed output of '%s'",
                      src_path);
      close(read_fd);
      /* Wait for child to reap it */
      int status_dummy;
      waitpid(pid, &status_dummy, 0);
      return false;
    }
  }

  if (n == -1) {
    otter_log_error(target->logger,
                    "Error reading preprocessor output for '%s': '%s'",
                    src_path, strerror(errno));
    close(read_fd);
    int status_dummy;
    waitpid(pid, &status_dummy, 0);
    return false;
  }

  close(read_fd);

  int status;
  if (waitpid(pid, &status, 0) == -1) {
    otter_log_error(target->logger,
                    "Error waiting for preprocessor for '%s': '%s'", src_path,
                    strerror(errno));
    return false;
  }

  if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
    otter_log_error(target->logger,
                    "Preprocessor (gcc -E) failed for '%s' with status %d",
                    src_path, WIFEXITED(status) ? WEXITSTATUS(status) : status);
    return false;
  }

  return true;
}

static bool otter_target_generate_hash_c(otter_target *target) {
  gnutls_hash_hd_t hash_hd;
  if (gnutls_hash_init(&hash_hd, GNUTLS_DIG_SHA1) < 0) {
    otter_log_critical(target->logger,
                       "Unable to create hash context for C target '%s'",
                       target->name);
    return false;
  }

  /* Ensure target->hash is NULL until allocated */
  target->hash = NULL;

  for (size_t i = 0; i < target->files_length; i++) {
    const char *src = target->files[i];
    otter_log_debug(target->logger, "Hashing file '%s'", src);
    if (!otter_preprocess_and_hash_file_posix_spawnp(target, hash_hd, src)) {
      otter_log_error(target->logger,
                      "Failed preprocessing+hashing of '%s' for target '%s'",
                      src, target->name);
      goto failure;
    }
  }

  target->hash_size = gnutls_hash_get_len(GNUTLS_DIG_SHA1);
  target->hash = otter_malloc(target->allocator, (size_t)target->hash_size);
  if (target->hash == NULL) {
    otter_log_critical(
        target->logger,
        "Unable to allocate buffer to store digest info for C target '%s'",
        target->name);
    goto failure;
  }

  gnutls_hash_deinit(hash_hd, target->hash);
  return true;

failure:
  /* If target->hash is NULL, gnutls will discard the digest; if non-NULL it
   * will be filled and left as-is */
  gnutls_hash_deinit(hash_hd, target->hash);
  return false;
}
