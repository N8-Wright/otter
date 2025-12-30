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
otter_vm *otter_vm_create(otter_allocator *allocator,
                          otter_bytecode *bytecode) {
  if (allocator == NULL || bytecode == NULL) {
    return NULL;
  }

  otter_vm *vm = otter_malloc(allocator, sizeof(*vm));
  if (vm == NULL) {
    return NULL;
  }

  vm->allocator = allocator;
  vm->bytecode = bytecode;
  vm->stack_index = 0;
  vm->stack =
      otter_malloc(allocator, sizeof(otter_object *) * OTTER_VM_STACK_SIZE);
  if (vm->stack == NULL) {
    otter_free(allocator, vm);
    return NULL;
  }

  return vm;
}

void otter_vm_free(otter_vm *vm) { otter_free(vm->allocator, vm); }
