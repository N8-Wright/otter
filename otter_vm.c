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

  otter_vm *vm = otter_malloc(allocator, sizeof(*vm));
  if (vm == NULL) {
    otter_log_critical(logger, "Unable to allocate %zd bytes for %s",
                       sizeof(*vm), OTTER_NAMEOF(vm));
    return NULL;
  }

  vm->allocator = allocator;
  vm->bytecode = bytecode;
  vm->logger = logger;
  vm->stack_index = 0;
  const size_t stack_size = sizeof(otter_object *) * OTTER_VM_STACK_SIZE;
  vm->stack = otter_malloc(allocator, stack_size);
  if (vm->stack == NULL) {
    otter_log_critical(logger, "Unable to allocate %zd bytes for %s",
                       stack_size, OTTER_NAMEOF(vm->stack));
    otter_free(allocator, vm);
    return NULL;
  }

  return vm;
}

void otter_vm_free(otter_vm *vm) {
  otter_free(vm->allocator, vm->stack);
  otter_free(vm->allocator, vm);
}
