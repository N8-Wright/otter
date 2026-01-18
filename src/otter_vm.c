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
#include "otter_vm.h"

#define OTTER_VM_STACK_SIZE 1024
otter_vm *otter_vm_create(otter_allocator *allocator, otter_bytecode *bytecode,
                          otter_logger *logger) {
  OTTER_RETURN_IF_NULL(logger, allocator, NULL);
  OTTER_RETURN_IF_NULL(logger, bytecode, NULL);
  OTTER_RETURN_IF_NULL(logger, logger, NULL);

  otter_vm *virtual_machine = otter_malloc(allocator, sizeof(*virtual_machine));
  if (virtual_machine == NULL) {
    otter_log_critical(logger, "Unable to allocate %zd bytes for %s",
                       sizeof(*virtual_machine), OTTER_NAMEOF(virtual_machine));
    return NULL;
  }

  virtual_machine->allocator = allocator;
  virtual_machine->bytecode = bytecode;
  virtual_machine->logger = logger;
  virtual_machine->stack_index = 0;
  const size_t stack_size = sizeof(otter_object *) * OTTER_VM_STACK_SIZE;
  virtual_machine->stack = otter_malloc(allocator, stack_size);
  if (virtual_machine->stack == NULL) {
    otter_log_critical(logger, "Unable to allocate %zd bytes for %s",
                       stack_size, OTTER_NAMEOF(virtual_machine->stack));
    otter_free(allocator, virtual_machine);
    return NULL;
  }

  return virtual_machine;
}

void otter_vm_free(otter_vm *virtual_machine) {
  otter_free(virtual_machine->allocator, virtual_machine->stack);
  otter_free(virtual_machine->allocator, virtual_machine);
}

OTTER_DEFINE_TRIVIAL_CLEANUP_FUNC(otter_vm *, otter_vm_free);

void otter_vm_run(otter_vm *virtual_machine) { (void)virtual_machine; }
