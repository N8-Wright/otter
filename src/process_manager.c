/*
  otter Copyright (C) 2026 Nathaniel Wright

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

#include "otter/process_manager.h"
#include "otter/cstring.h"

#include <assert.h>
#include <errno.h>
#include <spawn.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

extern char **environ;

typedef struct otter_process_manager_impl {
  otter_process_manager base;
  otter_allocator *allocator;
  otter_logger *logger;
} otter_process_manager_impl;

static void
otter_process_manager_free_impl(otter_process_manager *process_manager_) {
  if (process_manager_ == NULL) {
    return;
  }

  otter_process_manager_impl *process_manager =
      (otter_process_manager_impl *)process_manager_;
  otter_free(process_manager->allocator, process_manager);
}

static otter_process_id
otter_process_manager_queue_impl(otter_process_manager *process_manager_,
                                 otter_string *command) {
  otter_process_manager_impl *process_manager =
      (otter_process_manager_impl *)process_manager_;
  otter_process_id error_id = {.value = -1};
  otter_process_id result = error_id;
  char **argv = NULL;

  if (command == NULL) {
    otter_log_error(process_manager->logger, "Cannot queue NULL command");
    goto cleanup;
  }

  const char *delims = " \t\n";
  argv = otter_string_split_cstr(process_manager->allocator, command, delims);
  if (argv == NULL) {
    otter_log_error(process_manager->logger, "Failed to split command: '%s'",
                    otter_string_cstr(command));
    goto cleanup;
  }

  size_t argc = 0;
  while (argv[argc] != NULL) {
    argc++;
  }

  if (argc == 0) {
    otter_log_error(process_manager->logger, "Command is empty: '%s'",
                    otter_string_cstr(command));
    goto cleanup;
  }

  pid_t pid;
  int spawn_result = posix_spawnp(&pid, argv[0], NULL, NULL, argv, environ);

  if (spawn_result != 0) {
    otter_log_error(process_manager->logger,
                    "Failed to spawn process for command '%s': %s",
                    otter_string_cstr(command), strerror(spawn_result));
    goto cleanup;
  }

  otter_log_debug(process_manager->logger,
                  "Queued process %d for command: '%s'", pid,
                  otter_string_cstr(command));

  static_assert(sizeof(pid_t) == sizeof(int));
  memcpy(&result.value, &pid, sizeof(pid));

cleanup:
  if (argv != NULL) {
    for (size_t i = 0; argv[i] != NULL; i++) {
      otter_free(process_manager->allocator, argv[i]);
    }
    otter_free(process_manager->allocator, argv);
  }

  return result;
}

static void
otter_process_manager_wait_impl(otter_process_manager *process_manager_,
                                otter_process_id *ids, size_t ids_length,
                                int *exit_statuses) {
  otter_process_manager_impl *process_manager =
      (otter_process_manager_impl *)process_manager_;

  if (ids == NULL || ids_length == 0) {
    otter_log_debug(process_manager->logger, "No processes to wait for");
    return;
  }

  for (size_t i = 0; i < ids_length; i++) {
    if (ids[i].value < 0) {
      otter_log_error(process_manager->logger, "Invalid process ID: %d",
                      ids[i].value);
      if (exit_statuses != NULL) {
        exit_statuses[i] = -1;
      }
      continue;
    }

    pid_t pid;
    memcpy(&pid, &ids[i].value, sizeof(pid));

    int status;
    otter_log_debug(process_manager->logger, "Waiting for process %d", pid);

    if (waitpid(pid, &status, 0) == -1) {
      otter_log_error(process_manager->logger,
                      "Failed to wait for process %d: %s", pid,
                      strerror(errno));
      if (exit_statuses != NULL) {
        exit_statuses[i] = -1;
      }
      continue;
    }

    /* Store the full wait status if array provided */
    if (exit_statuses != NULL) {
      exit_statuses[i] = status;
    }

    if (WIFEXITED(status)) {
      int exit_status = WEXITSTATUS(status);
      if (exit_status == 0) {
        otter_log_debug(process_manager->logger,
                        "Process %d exited successfully", pid);
      } else {
        otter_log_error(process_manager->logger,
                        "Process %d exited with status %d", pid, exit_status);
      }
    } else if (WIFSIGNALED(status)) {
      otter_log_error(process_manager->logger,
                      "Process %d terminated by signal %d", pid,
                      WTERMSIG(status));
    } else {
      otter_log_error(process_manager->logger, "Process %d exited abnormally",
                      pid);
    }
  }
}

static otter_process_manager_vtable vtable = {
    .process_manager_free = otter_process_manager_free_impl,
    .process_manager_queue = otter_process_manager_queue_impl,
    .process_manager_wait = otter_process_manager_wait_impl,
};

otter_process_manager *otter_process_manager_create(otter_allocator *allocator,
                                                    otter_logger *logger) {
  if (allocator == NULL || logger == NULL) {
    return NULL;
  }

  otter_process_manager_impl *process_manager =
      otter_malloc(allocator, sizeof(*process_manager));
  if (process_manager == NULL) {
    otter_log_error(logger, "Failed to allocate process manager");
    return NULL;
  }

  process_manager->base.vtable = &vtable;
  process_manager->allocator = allocator;
  process_manager->logger = logger;

  return (otter_process_manager *)process_manager;
}

void otter_process_manager_free(otter_process_manager *process_manager) {
  if (process_manager == NULL || process_manager->vtable == NULL) {
    return;
  }

  process_manager->vtable->process_manager_free(process_manager);
}

OTTER_DEFINE_TRIVIAL_CLEANUP_FUNC(otter_process_manager *,
                                  otter_process_manager_free);

otter_process_id
otter_process_manager_queue(otter_process_manager *process_manager,
                            otter_string *command) {
  otter_process_id error_id = {.value = -1};

  if (process_manager == NULL || process_manager->vtable == NULL) {
    return error_id;
  }

  return process_manager->vtable->process_manager_queue(process_manager,
                                                        command);
}

void otter_process_manager_wait(otter_process_manager *process_manager,
                                otter_process_id *ids, size_t ids_length,
                                int *exit_statuses) {
  if (process_manager == NULL || process_manager->vtable == NULL) {
    return;
  }

  process_manager->vtable->process_manager_wait(process_manager, ids,
                                                ids_length, exit_statuses);
}
