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
#include "otter_allocator.h"
#include "otter_bytecode.h"
#include "otter_inc.h"
#include "otter_logger.h"
#include "otter_object.h"
typedef struct otter_vm {
  otter_allocator *allocator;
  otter_bytecode *bytecode;
  otter_logger *logger;
  size_t stack_index;
  otter_object **stack;
} otter_vm;

otter_vm *otter_vm_create(otter_allocator *allocator, otter_bytecode *bytecode,
                          otter_logger *logger);
void otter_vm_free(otter_vm *vm);
OTTER_DEFINE_TRIVIAL_CLEANUP_FUNC(otter_vm *, otter_vm_free);
void otter_vm_run(otter_vm *vm);
#endif /* OTTER_VM_ */
