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
#include "otter/allocator.h"
#include "otter/bytecode.h"
#include "otter/logger.h"
#include "otter/object.h"
#include "otter/vm.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

int main(void) {
  otter_allocator *allocator = otter_allocator_create();

  /* Test: Simple add operation */
  uint8_t instructions[] = {OP_CONSTANT, 0, OP_CONSTANT, 1, OP_ADD, OP_HALT};

  otter_bytecode *bytecode = otter_malloc(allocator, sizeof(*bytecode));
  bytecode->allocator = allocator;
  bytecode->constants = NULL;
  bytecode->constants_length = 0;
  bytecode->constants_capacity = 0;

  uint8_t *instr_copy = otter_malloc(allocator, sizeof(instructions));
  memcpy(instr_copy, instructions, sizeof(instructions));
  bytecode->instructions = (const char *)instr_copy;

  /* Create constants */
  otter_object_integer *const1 = otter_malloc(allocator, sizeof(*const1));
  const1->base.type = OTTER_OBJECT_TYPE_INTEGER;
  const1->value = 10;

  otter_object_integer *const2 = otter_malloc(allocator, sizeof(*const2));
  const2->base.type = OTTER_OBJECT_TYPE_INTEGER;
  const2->value = 32;

  bytecode->constants_capacity = 2;
  bytecode->constants_length = 2;
  bytecode->constants = otter_malloc(allocator, sizeof(otter_object *) * 2);
  bytecode->constants[0] = (otter_object *)const1;
  bytecode->constants[1] = (otter_object *)const2;

  otter_logger logger = {.vtable = NULL};
  otter_vm *virtual_machine = otter_vm_create(allocator, bytecode, &logger);

  otter_vm_run(virtual_machine);

  /* Check result */
  int success = 0;
  if (virtual_machine->stack_index == 1 &&
      virtual_machine->stack[0]->type == OTTER_OBJECT_TYPE_INTEGER &&
      ((otter_object_integer *)virtual_machine->stack[0])->value == 42) {
    printf("Test passed: 10 + 32 = 42\n");
    success = 1;
  } else {
    printf("Test failed\n");
  }

  /* Cleanup */
  otter_free(allocator, virtual_machine->stack[0]);
  otter_free(allocator, const1);
  otter_free(allocator, const2);
  otter_vm_free(virtual_machine);
  /* NOLINTBEGIN(cppcoreguidelines-pro-type-cppcast,cppcoreguidelines-pro-type-const-cast,performance-no-int-to-ptr)
   */
  otter_free(allocator, (char *)(uintptr_t)bytecode->instructions);
  /* NOLINTEND(cppcoreguidelines-pro-type-cppcast,cppcoreguidelines-pro-type-const-cast,performance-no-int-to-ptr)
   */
  otter_free(allocator, bytecode->constants);
  otter_free(allocator, bytecode);
  otter_allocator_free(allocator);

  return success ? 0 : 1;
}
