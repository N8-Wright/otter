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
#ifndef OTTER_PROCESS_MANAGER_
#define OTTER_PROCESS_MANAGER_
#include "allocator.h"
#include "inc.h"
#include "logger.h"
#include "string.h"
#include <stddef.h>

typedef struct otter_process_id {
  int value;
} otter_process_id;

typedef struct otter_process_manager otter_process_manager;
typedef struct otter_process_manager_vtable {
  void (*process_manager_free)(otter_process_manager *);
  otter_process_id (*process_manager_queue)(otter_process_manager *,
                                            otter_string *command);
  void (*process_manager_wait)(otter_process_manager *, otter_process_id *ids,
                               size_t ids_length, int *exit_statuses);
} otter_process_manager_vtable;

struct otter_process_manager {
  otter_process_manager_vtable *vtable;
};

otter_process_manager *otter_process_manager_create(otter_allocator *allocator,
                                                    otter_logger *logger);
void otter_process_manager_free(otter_process_manager *process_manager);
OTTER_DECLARE_TRIVIAL_CLEANUP_FUNC(otter_process_manager *,
                                   otter_process_manager_free);
otter_process_id
otter_process_manager_queue(otter_process_manager *process_manager,
                            otter_string *command);
void otter_process_manager_wait(otter_process_manager *process_manager,
                                otter_process_id *ids, size_t ids_length,
                                int *exit_statuses);
#endif /* OTTER_PROCESS_MANAGER_ */
