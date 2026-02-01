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
#include "otter/bytecode.h"
#include "otter/object.h"
#include "otter/test.h"
#include "otter/vm.h"
#include <stdint.h>
#include <string.h>

/* Helper to create a simple bytecode with instructions */
static otter_bytecode *create_bytecode(otter_allocator *allocator,
                                       const uint8_t *instructions,
                                       size_t instruction_count) {
  otter_bytecode *bytecode = otter_malloc(allocator, sizeof(*bytecode));
  bytecode->allocator = allocator;
  OTTER_ARRAY_INIT(bytecode, constants, allocator);

  uint8_t *instr_copy = otter_malloc(allocator, instruction_count);
  memcpy(instr_copy, instructions, instruction_count);
  bytecode->instructions = (const char *)instr_copy;

  return bytecode;
}

/* Helper to add a constant to bytecode */
static void bytecode_add_constant(otter_bytecode *bytecode,
                                  otter_object *constant) {
  OTTER_ARRAY_APPEND(bytecode, constants, bytecode->allocator, constant);
}

/* Helper to free bytecode */
static void free_bytecode(otter_bytecode *bytecode) {
  /* NOLINTBEGIN(cppcoreguidelines-pro-type-cppcast,cppcoreguidelines-pro-type-const-cast,performance-no-int-to-ptr)
   */
  otter_free(bytecode->allocator, (char *)(uintptr_t)bytecode->instructions);
  /* NOLINTEND(cppcoreguidelines-pro-type-cppcast,cppcoreguidelines-pro-type-const-cast,performance-no-int-to-ptr)
   */
  if (bytecode->constants != NULL) {
    otter_free(bytecode->allocator, bytecode->constants);
  }
  otter_free(bytecode->allocator, bytecode);
}

OTTER_TEST(vm_create_succeeds) {
  uint8_t instructions[] = {OP_HALT};
  otter_bytecode *bytecode =
      create_bytecode(OTTER_TEST_ALLOCATOR, instructions, sizeof(instructions));

  otter_logger logger = {.vtable = NULL};
  otter_vm *virtual_machine =
      otter_vm_create(OTTER_TEST_ALLOCATOR, bytecode, &logger);

  OTTER_ASSERT(virtual_machine != NULL);
  OTTER_ASSERT(virtual_machine->stack != NULL);
  OTTER_ASSERT(virtual_machine->stack_index == 0);
  OTTER_ASSERT(virtual_machine->bytecode == bytecode);

  OTTER_TEST_END(otter_vm_free(virtual_machine); free_bytecode(bytecode););
}

OTTER_TEST(vm_op_halt) {
  uint8_t instructions[] = {OP_HALT};
  otter_bytecode *bytecode =
      create_bytecode(OTTER_TEST_ALLOCATOR, instructions, sizeof(instructions));

  otter_logger logger = {.vtable = NULL};
  otter_vm *virtual_machine =
      otter_vm_create(OTTER_TEST_ALLOCATOR, bytecode, &logger);

  otter_vm_run(virtual_machine);

  /* If we get here, HALT worked */
  OTTER_ASSERT(true);

  OTTER_TEST_END(otter_vm_free(virtual_machine); free_bytecode(bytecode););
}

OTTER_TEST(vm_op_constant) {
  const int expected_value = 42;

  uint8_t instructions[] = {OP_CONSTANT, 0, OP_HALT};
  otter_bytecode *bytecode =
      create_bytecode(OTTER_TEST_ALLOCATOR, instructions, sizeof(instructions));

  /* Add a constant to the pool */
  otter_object_integer *constant =
      otter_malloc(OTTER_TEST_ALLOCATOR, sizeof(*constant));
  constant->base.type = OTTER_OBJECT_TYPE_INTEGER;
  constant->value = expected_value;

  bytecode_add_constant(bytecode, (otter_object *)constant);

  otter_logger logger = {.vtable = NULL};
  otter_vm *virtual_machine =
      otter_vm_create(OTTER_TEST_ALLOCATOR, bytecode, &logger);

  otter_vm_run(virtual_machine);

  /* Check that constant was pushed to stack */
  OTTER_ASSERT(virtual_machine->stack_index == 1);
  OTTER_ASSERT(virtual_machine->stack[0] == (otter_object *)constant);
  OTTER_ASSERT(((otter_object_integer *)virtual_machine->stack[0])->value ==
               expected_value);

  OTTER_TEST_END(otter_free(OTTER_TEST_ALLOCATOR, constant);
                 otter_vm_free(virtual_machine); free_bytecode(bytecode););
}
