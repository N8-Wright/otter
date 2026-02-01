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
#ifndef OTTER_VM_
#define OTTER_VM_
#include "allocator.h"
#include "bytecode.h"
#include "inc.h"
#include "logger.h"
#include "object.h"
typedef struct otter_vm {
  otter_allocator *allocator;
  otter_bytecode *bytecode;
  otter_logger *logger;
  size_t stack_index;
  otter_object **stack;
  otter_object *objects; /* Head of linked list of all allocated objects */
} otter_vm;

otter_vm *otter_vm_create(otter_allocator *allocator, otter_bytecode *bytecode,
                          otter_logger *logger);
void otter_vm_free(otter_vm *virtual_machine);
OTTER_DECLARE_TRIVIAL_CLEANUP_FUNC(otter_vm *, otter_vm_free);
void otter_vm_run(otter_vm *virtual_machine);
#endif /* OTTER_VM_ */
