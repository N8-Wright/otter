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
#include "otter/target.h"
#include "otter/cstring.h"

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
    otter_log_debug(target->logger, "'%s' was executed",
                    otter_string_cstr(target->name));
    *executed = true;
  } else {
    otter_log_debug(target->logger, "'%s' was not executed",
                    otter_string_cstr(target->name));
  }

  OTTER_ARRAY_FOREACH(target, dependencies, otter_target_was_executed,
                      executed);
}

static bool otter_target_needs_execute(otter_target *target) {
  otter_log_debug(target->logger, "Checking if '%s' needs to be executed",
                  otter_string_cstr(target->name));
  bool any_dependency_executed = false;
  OTTER_ARRAY_FOREACH(target, dependencies, otter_target_was_executed,
                      &any_dependency_executed);
  if (any_dependency_executed) {
    if (target->type != OTTER_TARGET_OBJECT) {
      otter_log_debug(target->logger,
                      "'%s' target needs to execute because one or more of its "
                      "dependencies was exectued",
                      otter_string_cstr(target->name));
      return true;
    }
    otter_log_debug(target->logger,
                    "One or more of %s's dependencies was executed.  This "
                    "not enough to say that '%s' needs to execute though",
                    otter_string_cstr(target->name),
                    otter_string_cstr(target->name));
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
      target->filesystem, otter_string_cstr(target->name), OTTER_XATTR_NAME,
      stored_hash, expected_hash_size);
  if (stored_hash_size < 0) {
    otter_free(target->allocator, stored_hash);
    return true;
  }

  if ((unsigned int)stored_hash_size != target->hash_size) {
    otter_log_debug(
        target->logger,
        "Hashes do not match for target '%s'.  It needs to be executed.",
        otter_string_cstr(target->name));
    otter_free(target->allocator, stored_hash);
    return true;
  }

  if (memcmp(target->hash, stored_hash, target->hash_size) == 0) {
    otter_log_debug(
        target->logger,
        "Hashes match for target '%s'.  It does not need to be executed.",
        otter_string_cstr(target->name));
    otter_free(target->allocator, stored_hash);
    return false;
  }
  otter_log_debug(
      target->logger,
      "Hashes do not match for target '%s'.  It needs to be executed.",
      otter_string_cstr(target->name));
  otter_free(target->allocator, stored_hash);
  return true;
}

static void otter_target_store_hash(otter_target *target) {
  // Store raw digest in xattr
  assert(target->hash != NULL);
  assert(target->hash_size != 0);
  if (otter_filesystem_set_attribute(
          target->filesystem, otter_string_cstr(target->name), OTTER_XATTR_NAME,
          target->hash, target->hash_size) < 0) {
    otter_log_error(
        target->logger, "Failed to set %s attribute on file '%s': '%s'\n",
        OTTER_XATTR_NAME, otter_string_cstr(target->name), strerror(errno));

    return;
  }
}

static int otter_cc_check_available(otter_logger *logger) {
  static int cached_result =
      -1; /* -1 = unchecked, 0 = available, 1 = unavailable */

  if (cached_result != -1) {
    return cached_result;
  }

  pid_t pid;
  char *const argv[] = {"cc", "--version", NULL};
  const int spawn_result = posix_spawnp(&pid, "cc", NULL, NULL, argv, environ);

  if (spawn_result != 0) {
    otter_log_error(logger, "C compiler (cc) is not installed or not in PATH");
    cached_result = 1;
    return cached_result;
  }

  int status;
  if (waitpid(pid, &status, 0) > 0 && WIFEXITED(status)) {
    cached_result = 0;
  } else {
    otter_log_error(logger, "C compiler (cc) is not installed or not in PATH");
    cached_result = 1;
  }

  return cached_result;
}

static int otter_clang_tidy_check_available(otter_logger *logger) {
  static int cached_result =
      -1; /* -1 = unchecked, 0 = available, 1 = unavailable */

  if (cached_result != -1) {
    return cached_result;
  }

  pid_t pid;
  char *const argv[] = {"clang-tidy", "--version", NULL};
  const int spawn_result =
      posix_spawnp(&pid, "clang-tidy", NULL, NULL, argv, environ);

  if (spawn_result != 0) {
    otter_log_error(logger, "clang-tidy is not installed or not in PATH");
    cached_result = 1;
    return cached_result;
  }

  int status;
  if (waitpid(pid, &status, 0) > 0 && WIFEXITED(status) &&
      WEXITSTATUS(status) == 0) {
    cached_result = 0;
  } else {
    otter_log_error(logger, "clang-tidy is not installed or not in PATH");
    cached_result = 1;
  }

  return cached_result;
}

static int otter_target_run_clang_tidy(otter_target *target) {
  if (target == NULL) {
    return -1;
  }

  /* Only run clang-tidy on targets that have source files */
  if (OTTER_ARRAY_LENGTH(target, files) == 0) {
    return 0;
  }

  /* Check if clang-tidy is available before proceeding */
  if (otter_clang_tidy_check_available(target->logger) != 0) {
    return -1;
  }

  int result = -1;
  char **argv = NULL;
  char **include_flag_tokens = NULL;
  size_t arg_count = 0;

  /* Split include flags */
  size_t flag_count = 0;
  if (target->include_flags != NULL) {
    include_flag_tokens = otter_string_split_cstr(
        target->allocator, target->include_flags, " \t\n");
    if (include_flag_tokens == NULL) {
      otter_log_error(target->logger, "Failed to split include flags");
      return -1;
    }
    /* Count tokens */
    for (size_t i = 0; include_flag_tokens[i] != NULL; i++) {
      flag_count++;
    }
  }

  /* Build argv: clang-tidy <files...> -- <include_flags> NULL */
  const size_t file_count = OTTER_ARRAY_LENGTH(target, files);
  const size_t argc = 1 + file_count + 1 + flag_count +
                      1; /* clang-tidy + files + "--" + include flags + NULL */
  argv = (char **)otter_malloc(target->allocator, argc * sizeof(char *));
  if (argv == NULL) {
    otter_log_error(target->logger, "Failed to allocate argv for clang-tidy");
    if (include_flag_tokens != NULL) {
      for (size_t i = 0; include_flag_tokens[i] != NULL; i++) {
        otter_free(target->allocator, include_flag_tokens[i]);
      }
      otter_free(target->allocator, include_flag_tokens);
    }
    return -1;
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
    argv[arg_count] = otter_strdup(
        target->allocator,
        otter_string_cstr(OTTER_ARRAY_AT_UNSAFE(target, files, i)));
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

  /* Add include flags after -- */
  if (include_flag_tokens != NULL) {
    for (size_t i = 0; include_flag_tokens[i] != NULL; i++) {
      argv[arg_count] = otter_strdup(target->allocator, include_flag_tokens[i]);
      if (argv[arg_count] == NULL) {
        goto cleanup;
      }
      arg_count++;
    }
  }

  otter_log_info(target->logger, "Running clang-tidy on target '%s'",
                 otter_string_cstr(target->name));

  pid_t pid;
  const int posix_spawn_result =
      posix_spawnp(&pid, "clang-tidy", NULL, NULL, argv, environ);

  if (posix_spawn_result != 0) {
    otter_log_error(
        target->logger, "Failed to spawn clang-tidy for target '%s': '%s'",
        otter_string_cstr(target->name), strerror(posix_spawn_result));
    goto cleanup;
  }

  int status;
  if (waitpid(pid, &status, 0) > 0) {
    if (!WIFEXITED(status)) {
      otter_log_error(target->logger, "clang-tidy failed for target '%s'",
                      otter_string_cstr(target->name));
      goto cleanup;
    }
    result = WEXITSTATUS(status);
  }

cleanup:
  if (argv != NULL) {
    for (size_t i = 0; i < argc; i++) {
      otter_free(target->allocator, argv[i]);
    }
    otter_free(target->allocator, (void *)argv);
  }
  if (include_flag_tokens != NULL) {
    for (size_t i = 0; include_flag_tokens[i] != NULL; i++) {
      otter_free(target->allocator, include_flag_tokens[i]);
    }
    otter_free(target->allocator, include_flag_tokens);
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

/* Helper to build char** array from otter_string* array for
 * posix_spawnp Note: The strings themselves are const, but
 * posix_spawnp signature requires char *const argv[] (array of
 * mutable pointers to mutable char). We know posix_spawnp won't
 * modify the strings, so this is safe. */
static char **otter_target_build_cstr_argv(otter_target *target) {
  const size_t argc = OTTER_ARRAY_LENGTH(target, argv);
  if (argc <= 1) {
    return NULL;
  }

  char **cstr_argv =
      otter_malloc(target->allocator, (argc + 1) * sizeof(char *));
  if (cstr_argv == NULL) {
    return NULL;
  }

  for (size_t i = 0; i < argc; i++) {
    otter_string *str = OTTER_ARRAY_AT_UNSAFE(target, argv, i);
    if (str == NULL) {
      return NULL;
    }

    /* POSIX guarantees posix_spawnp won't modify argv strings */
    union {
      const char *c;
      char *m;
    } cast;
    cast.c = otter_string_cstr(str);
    cstr_argv[i] = cast.m;
  }

  cstr_argv[argc] = NULL;
  return cstr_argv;
}

static int otter_target_execute_dependency(otter_target *target) {
  if (target == NULL) {
    return -1;
  }

  if (otter_cc_check_available(target->logger) != 0) {
    return -1;
  }

  otter_log_debug(target->logger, "%s: attempting to execute '%s'", __func__,
                  otter_string_cstr(target->name));
  int return_code = 0;
  OTTER_ARRAY_FOREACH(target, dependencies,
                      otter_target_execute_dependency_helper, &return_code);
  if (return_code != 0) {
    return return_code;
  }

  if (otter_target_needs_execute(target)) {
    target->executed = true;
    otter_log_info(target->logger, "Executing target '%s'\nCommand: '%s'",
                   otter_string_cstr(target->name),
                   otter_string_cstr(target->command));

    int clang_tidy_result = otter_target_run_clang_tidy(target);
    if (clang_tidy_result != 0) {
      otter_log_error(target->logger, "clang-tidy failed for target '%s'",
                      otter_string_cstr(target->name));
      return clang_tidy_result;
    }

    char **cstr_argv = otter_target_build_cstr_argv(target);
    if (cstr_argv == NULL) {
      otter_log_error(target->logger, "Failed to build argv array");
      return -1;
    }

    pid_t pid;
    int posix_spawn_result =
        posix_spawnp(&pid, cstr_argv[0], NULL, NULL, cstr_argv, environ);
    otter_free(target->allocator, cstr_argv);

    if (posix_spawn_result == 0) {
      int status;
      if (waitpid(pid, &status, 0) > 0) {
        if (!WIFEXITED(status)) {
          otter_log_error(
              target->logger, "Failed in execution of command %s\n",
              otter_string_cstr(OTTER_ARRAY_AT_UNSAFE(target, argv, 0)));
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
    }
    otter_log_error(target->logger,
                    "Failed to spawn target process %s beacuse '%s'\n",
                    otter_string_cstr(OTTER_ARRAY_AT_UNSAFE(target, argv, 0)),
                    strerror(posix_spawn_result));

    return -1;
  }

  return 0;
}

int otter_target_execute(otter_target *target) {
  if (target == NULL) {
    return -1;
  }

  if (otter_cc_check_available(target->logger) != 0) {
    return -1;
  }

  int return_code = 0;
  OTTER_ARRAY_FOREACH(target, dependencies,
                      otter_target_execute_dependency_helper, &return_code);
  if (return_code != 0) {
    return return_code;
  }

  if (otter_target_needs_execute(target)) {
    int clang_tidy_result = otter_target_run_clang_tidy(target);
    if (clang_tidy_result != 0) {
      otter_log_error(target->logger, "clang-tidy failed for target '%s'",
                      otter_string_cstr(target->name));
      return clang_tidy_result;
    }

    target->executed = true;
    otter_log_info(target->logger, "Executing target '%s'\nCommand: '%s'",
                   otter_string_cstr(target->name),
                   otter_string_cstr(target->command));

    char **cstr_argv = otter_target_build_cstr_argv(target);
    if (cstr_argv == NULL) {
      otter_log_error(target->logger, "Failed to build argv array");
      return -1;
    }

    pid_t pid;
    int posix_spawn_result =
        posix_spawnp(&pid, cstr_argv[0], NULL, NULL, cstr_argv, environ);
    otter_free(target->allocator, cstr_argv);

    if (posix_spawn_result == 0) {
      int status;
      if (waitpid(pid, &status, 0) > 0) {
        if (!WIFEXITED(status)) {
          otter_log_error(
              target->logger, "Failed in execution of command %s\n",
              otter_string_cstr(OTTER_ARRAY_AT_UNSAFE(target, argv, 0)));
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
    }
    otter_log_error(target->logger,
                    "Failed to spawn target process %s beacuse '%s'\n",
                    otter_string_cstr(OTTER_ARRAY_AT_UNSAFE(target, argv, 0)),
                    strerror(posix_spawn_result));

    return -1;
  }

  otter_log_info(target->logger, "Target '%s' up-to-date",
                 otter_string_cstr(target->name));
  return 0;
}

void otter_target_free(otter_target *target) {
  if (target == NULL) {
    return;
  }

  otter_string_free(target->name);
  if (target->files != NULL) {
    OTTER_ARRAY_FOREACH(target, files, otter_string_free);
  }
  otter_free(target->allocator, target->files);
  otter_string_free(target->command);
  otter_string_free(target->cc_flags);
  otter_string_free(target->include_flags);
  if (target->argv != NULL) {
    OTTER_ARRAY_FOREACH(target, argv, otter_string_free);
  }
  otter_free(target->allocator, target->argv);
  otter_free(target->allocator, target->dependencies);
  otter_free(target->allocator, target->hash);
  otter_free(target->allocator, target);
}

OTTER_DEFINE_TRIVIAL_CLEANUP_FUNC(otter_target *, otter_target_free);

static bool otter_target_generate_command_from_argv(otter_target *target) {
  target->command = otter_string_create(target->allocator, "", 0);
  if (target->command == NULL) {
    return false;
  }

  for (size_t i = 0; i < OTTER_ARRAY_LENGTH(target, argv); i++) {
    otter_string *arg = OTTER_ARRAY_AT_UNSAFE(target, argv, i);
    if (i > 0) {
      otter_string_append_cstr(&target->command, " ");
    }
    otter_string_append(&target->command, otter_string_cstr(arg),
                        otter_string_length(arg));
  }

  return true;
}

static bool otter_target_append_arg_to_argv(otter_target *target,
                                            const char *arg_) {
  if (target == NULL || arg_ == NULL) {
    return false;
  }

  for (size_t i = 0; i < OTTER_ARRAY_LENGTH(target, argv); i++) {
    otter_string *existing_arg = OTTER_ARRAY_AT_UNSAFE(target, argv, i);
    if (0 == otter_string_compare_cstr(existing_arg, arg_)) {
      otter_log_debug(
          target->logger,
          "Skipping adding argument '%s' to argv since it already exists",
          arg_);
      return true;
    }
  }

  otter_string *arg = otter_string_from_cstr(target->allocator, arg_);
  if (arg == NULL) {
    otter_log_critical(target->logger,
                       "Failed to create string for argument '%s'", arg_);
    return false;
  }

  if (!OTTER_ARRAY_APPEND(target, argv, target->allocator, arg)) {
    otter_log_critical(target->logger,
                       "Failed to append argument '%s' to array %s", arg_,
                       OTTER_NAMEOF(target->argv));
    otter_string_free(arg);
    return false;
  }

  return true;
}

static bool otter_target_append_args_to_argv(otter_target *target,
                                             const char *args_) {
  const char *delims = " \t\n";

  /* Create temporary otter_string for splitting */
  otter_string *args_str = otter_string_from_cstr(target->allocator, args_);
  if (args_str == NULL) {
    otter_log_critical(target->logger,
                       "Failed to create string from arguments '%s'", args_);
    return false;
  }

  char **tokens = otter_string_split_cstr(target->allocator, args_str, delims);
  otter_string_free(args_str);

  if (tokens == NULL) {
    otter_log_critical(target->logger, "Failed to split arguments '%s'", args_);
    return false;
  }

  for (size_t i = 0; tokens[i] != NULL; i++) {
    if (!otter_target_append_arg_to_argv(target, tokens[i])) {
      for (size_t j = 0; tokens[j] != NULL; j++) {
        otter_free(target->allocator, tokens[j]);
      }
      otter_free(target->allocator, tokens);
      return false;
    }
  }

  for (size_t i = 0; tokens[i] != NULL; i++) {
    otter_free(target->allocator, tokens[i]);
  }
  otter_free(target->allocator, tokens);
  return true;
}

static bool otter_target_generate_c_object_argv(otter_target *target,
                                                const otter_string *cc_flags) {
  if (target == NULL || cc_flags == NULL) {
    return false;
  }

  if (!otter_target_append_args_to_argv(target, "cc -fPIC -c")) {
    return false;
  }

  for (size_t i = 0; i < OTTER_ARRAY_LENGTH(target, files); i++) {
    if (!otter_target_append_arg_to_argv(
            target,
            otter_string_cstr(OTTER_ARRAY_AT_UNSAFE(target, files, i)))) {
      return false;
    }
  }

  if (!otter_target_append_arg_to_argv(target, "-o")) {
    return false;
  }

  if (!otter_target_append_arg_to_argv(target,
                                       otter_string_cstr(target->name))) {
    return false;
  }

  if (target->include_flags != NULL) {
    if (!otter_target_append_args_to_argv(
            target, otter_string_cstr(target->include_flags))) {
      return false;
    }
  }

  if (!otter_target_append_args_to_argv(target, otter_string_cstr(cc_flags))) {
    return false;
  }

  return otter_target_generate_command_from_argv(target);
}

static bool otter_target_append_objects_to_argv(otter_target *target,
                                                otter_target *dependency) {
  if (dependency->type == OTTER_TARGET_OBJECT) {
    if (!otter_target_append_arg_to_argv(target,
                                         otter_string_cstr(dependency->name))) {
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

static bool
otter_target_generate_c_executable_argv(otter_target *target,
                                        const otter_string *cc_flags) {
  if (target == NULL || cc_flags == NULL) {
    return false;
  }

  if (!otter_target_append_args_to_argv(target, "cc -o")) {
    return false;
  }

  if (!otter_target_append_arg_to_argv(target,
                                       otter_string_cstr(target->name))) {
    return false;
  }

  for (size_t i = 0; i < OTTER_ARRAY_LENGTH(target, files); i++) {
    if (!otter_target_append_arg_to_argv(
            target,
            otter_string_cstr(OTTER_ARRAY_AT_UNSAFE(target, files, i)))) {
      return false;
    }
  }

  for (size_t i = 0; i < OTTER_ARRAY_LENGTH(target, dependencies); i++) {
    if (!otter_target_append_objects_to_argv(
            target, OTTER_ARRAY_AT_UNSAFE(target, dependencies, i))) {
      return false;
    }
  }

  if (target->include_flags != NULL) {
    if (!otter_target_append_args_to_argv(
            target, otter_string_cstr(target->include_flags))) {
      return false;
    }
  }

  if (!otter_target_append_args_to_argv(target, otter_string_cstr(cc_flags))) {
    return false;
  }

  return otter_target_generate_command_from_argv(target);
}

static bool
otter_target_generate_c_shared_object_argv(otter_target *target,
                                           const otter_string *cc_flags) {
  if (target == NULL || cc_flags == NULL) {
    return false;
  }

  if (!otter_target_append_args_to_argv(target, "cc -shared -fPIC -o")) {
    return false;
  }

  if (!otter_target_append_arg_to_argv(target,
                                       otter_string_cstr(target->name))) {
    return false;
  }

  for (size_t i = 0; i < OTTER_ARRAY_LENGTH(target, files); i++) {
    if (!otter_target_append_arg_to_argv(
            target,
            otter_string_cstr(OTTER_ARRAY_AT_UNSAFE(target, files, i)))) {
      return false;
    }
  }

  for (size_t i = 0; i < OTTER_ARRAY_LENGTH(target, dependencies); i++) {
    if (!otter_target_append_objects_to_argv(
            target, OTTER_ARRAY_AT_UNSAFE(target, dependencies, i))) {
      return false;
    }
  }

  if (target->include_flags != NULL) {
    if (!otter_target_append_args_to_argv(
            target, otter_string_cstr(target->include_flags))) {
      return false;
    }
  }

  if (!otter_target_append_args_to_argv(target, otter_string_cstr(cc_flags))) {
    return false;
  }

  return otter_target_generate_command_from_argv(target);
}

static otter_target *otter_target_create_and_initialize(
    const otter_string *name, otter_allocator *allocator,
    otter_filesystem *filesystem, otter_logger *logger,
    otter_target_type type) {
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
  target->cc_flags = NULL;
  target->include_flags = NULL;
  target->argv = NULL;
  target->dependencies = NULL;
  target->hash = NULL;
  target->hash_size = 0;
  target->executed = false;
  target->type = type;

  target->name = otter_string_copy(name);
  if (target->name == NULL) {
    otter_log_critical(target->logger, "Failed to create string %s",
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

otter_target *otter_target_create_c_object(const otter_string *name,
                                           const otter_string *cc_flags,
                                           const otter_string *include_flags,
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

  target->cc_flags = otter_string_copy(cc_flags);
  if (target->cc_flags == NULL) {
    otter_log_critical(logger, "Failed to create cc_flags string");
    goto failure;
  }

  target->include_flags = otter_string_copy(include_flags);
  if (target->include_flags == NULL) {
    otter_log_critical(logger, "Failed to create include_flags string");
    goto failure;
  }

  va_list args;
  va_start(args, logger);
  otter_string *file = va_arg(args, otter_string *);
  while (file != NULL) {
    otter_string *duplicated_file = otter_string_copy(file);
    if (duplicated_file == NULL) {
      otter_log_critical(target->logger,
                         "Failed to create string for file: '%s'",
                         otter_string_cstr(file));
      va_end(args);
      goto failure;
    }

    if (!OTTER_ARRAY_APPEND(target, files, target->allocator,
                            duplicated_file)) {
      otter_log_critical(target->logger,
                         "Failed to insert file string '%s' into %s",
                         otter_string_cstr(file), OTTER_NAMEOF(target->files));
      va_end(args);
      goto failure;
    }

    file = va_arg(args, otter_string *);
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
    const otter_string *name, const otter_string *flags,
    const otter_string *include_flags, otter_allocator *allocator,
    otter_filesystem *filesystem, otter_logger *logger,
    const otter_string **files, otter_target **dependencies) {
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

  target->cc_flags = otter_string_copy(flags);
  if (target->cc_flags == NULL) {
    otter_log_critical(logger, "Failed to create cc_flags string");
    goto failure;
  }

  target->include_flags = otter_string_copy(include_flags);
  if (target->include_flags == NULL) {
    otter_log_critical(logger, "Failed to create include_flags string");
    goto failure;
  }

  const otter_string **file = files;
  while (*file != NULL) {
    otter_string *duplicated_file = otter_string_copy(*file);
    if (duplicated_file == NULL) {
      otter_log_critical(target->logger, "Unable to create string for '%s'",
                         otter_string_cstr(*file));
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
    const otter_string *name, const otter_string *flags,
    const otter_string *include_flags, otter_allocator *allocator,
    otter_filesystem *filesystem, otter_logger *logger,
    const otter_string **files, otter_target **dependencies) {
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

  target->cc_flags = otter_string_copy(flags);
  if (target->cc_flags == NULL) {
    otter_log_critical(logger, "Failed to create cc_flags string");
    goto failure;
  }

  target->include_flags = otter_string_copy(include_flags);
  if (target->include_flags == NULL) {
    otter_log_critical(logger, "Failed to create include_flags string");
    goto failure;
  }

  const otter_string **file = files;
  while (*file != NULL) {
    otter_string *duplicated_file = otter_string_copy(*file);
    if (duplicated_file == NULL) {
      otter_log_critical(target->logger, "Unable to create string for '%s'",
                         otter_string_cstr(*file));
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

void otter_target_add_command(otter_target *target,
                              const otter_string *command_) {
  if (target == NULL) {
    return;
  }

  if (command_ == NULL) {
    return;
  }

  target->command = otter_string_copy(command_);
  const char *delims = " \t\n";

  otter_string **tokens =
      otter_string_split(target->allocator, command_, delims);
  if (tokens == NULL) {
    return;
  }

  for (size_t i = 0; tokens[i] != NULL; i++) {
    if (!OTTER_ARRAY_APPEND(target, argv, target->allocator, tokens[i])) {
      /* On failure, free remaining tokens and clear argv */
      for (size_t j = i; tokens[j] != NULL; j++) {
        otter_string_free(tokens[j]);
      }
      otter_free(target->allocator, tokens);
      OTTER_ARRAY_FOREACH(target, argv, otter_string_free);
      return;
    }
  }

  /* Free the tokens array (but not the strings themselves - they're now in
   * argv) */
  otter_free(target->allocator, tokens);
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

  /* Build argv: cc -E -P <include_flags> path NULL */
  /* Split include flags */
  char **include_flag_tokens = NULL;
  size_t flag_count = 0;
  if (target->include_flags != NULL) {
    include_flag_tokens = otter_string_split_cstr(
        target->allocator, target->include_flags, " \t\n");
    if (include_flag_tokens != NULL) {
      /* Count tokens */
      for (size_t i = 0; include_flag_tokens[i] != NULL; i++) {
        flag_count++;
      }
    }
  }

  /* Allocate argv: cc + -E + -P + flags + path + NULL */
  size_t argc = 3 + flag_count + 1 + 1;
  char **argv = otter_malloc(target->allocator, argc * sizeof(char *));
  if (argv == NULL) {
    otter_free(target->allocator, path);
    if (include_flag_tokens != NULL) {
      for (size_t i = 0; include_flag_tokens[i] != NULL; i++) {
        otter_free(target->allocator, include_flag_tokens[i]);
      }
      otter_free(target->allocator, include_flag_tokens);
    }
    return false;
  }

  /* Initialize all to NULL for safe cleanup */
  for (size_t i = 0; i < argc; i++) {
    argv[i] = NULL;
  }

  size_t arg_idx = 0;
  argv[arg_idx++] = otter_strdup(target->allocator, "cc");
  argv[arg_idx++] = otter_strdup(target->allocator, "-E");
  argv[arg_idx++] = otter_strdup(target->allocator, "-P");

  /* Add include flags */
  if (include_flag_tokens != NULL) {
    for (size_t i = 0; include_flag_tokens[i] != NULL && arg_idx < argc - 2;
         i++) {
      argv[arg_idx++] = otter_strdup(target->allocator, include_flag_tokens[i]);
    }
  }

  argv[arg_idx++] = path; /* Use path directly, don't dup again */
  argv[arg_idx] = NULL;

  /* Free the token array (tokens were duplicated into argv) */
  if (include_flag_tokens != NULL) {
    for (size_t i = 0; include_flag_tokens[i] != NULL; i++) {
      otter_free(target->allocator, include_flag_tokens[i]);
    }
    otter_free(target->allocator, include_flag_tokens);
  }

  pid_t pid;
  int spawn_err = posix_spawnp(&pid, "cc", &actions, NULL, argv, environ);

  /* actions may be destroyed regardless of spawn success */
  posix_spawn_file_actions_destroy(&actions);

  /* Free argv array and all duplicated strings except path (freed below) */
  for (size_t i = 0; i < arg_idx; i++) {
    if (argv[i] != path) {
      otter_free(target->allocator, argv[i]);
    }
  }
  otter_free(target->allocator, argv);
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

  static const size_t buffer_size = 4096;
  unsigned char buffer[buffer_size];
  ssize_t bytes_read;
  while ((bytes_read = read(read_fd, buffer, sizeof(buffer))) > 0) {
    if (gnutls_hash(hash_hd, buffer, (size_t)bytes_read) < 0) {
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

  if (bytes_read == -1) {
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
    const char *src = otter_string_cstr(target->files[i]);
    otter_log_debug(target->logger, "Hashing file '%s'", src);
    if (!otter_preprocess_and_hash_file_posix_spawnp(target, hash_hd, src)) {
      otter_log_error(target->logger,
                      "Failed preprocessing+hashing of '%s' for target '%s'",
                      src, otter_string_cstr(target->name));
      goto failure;
    }
  }

  target->hash_size = gnutls_hash_get_len(GNUTLS_DIG_SHA1);
  target->hash = otter_malloc(target->allocator, (size_t)target->hash_size);
  if (target->hash == NULL) {
    otter_log_critical(
        target->logger,
        "Unable to allocate buffer to store digest info for C target '%s'",
        otter_string_cstr(target->name));
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
