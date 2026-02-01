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

/* Test: OP_JUMP jumps forward */
OTTER_TEST(vm_op_jump) {
  uint8_t instructions[] = {OP_TRUE,       /* push true */
                            OP_JUMP, 0, 1, /* skip 1 byte */
                            OP_POP,        /* this should be skipped */
                            OP_HALT};
  otter_bytecode *bytecode =
      create_bytecode(OTTER_TEST_ALLOCATOR, instructions, sizeof(instructions));

  otter_logger logger = {.vtable = NULL};
  otter_vm *virtual_machine =
      otter_vm_create(OTTER_TEST_ALLOCATOR, bytecode, &logger);

  otter_vm_run(virtual_machine);

  /* If jump worked, we should have 1 item on stack (true was not popped) */
  OTTER_ASSERT(virtual_machine->stack_index == 1);
  OTTER_ASSERT(virtual_machine->stack[0]->type == OTTER_OBJECT_TYPE_BOOL);

  OTTER_TEST_END(otter_vm_free(virtual_machine); free_bytecode(bytecode););
}

/* Test: OP_JUMP_IF_FALSE jumps when false */
OTTER_TEST(vm_op_jump_if_false_takes_jump) {
  uint8_t instructions[] = {OP_FALSE,               /* push false */
                            OP_JUMP_IF_FALSE, 0, 1, /* jump 1 byte */
                            OP_POP,                 /* this should be skipped */
                            OP_HALT};
  otter_bytecode *bytecode =
      create_bytecode(OTTER_TEST_ALLOCATOR, instructions, sizeof(instructions));

  otter_logger logger = {.vtable = NULL};
  otter_vm *virtual_machine =
      otter_vm_create(OTTER_TEST_ALLOCATOR, bytecode, &logger);

  otter_vm_run(virtual_machine);

  /* If jump worked, false is still on stack (was not popped) */
  OTTER_ASSERT(virtual_machine->stack_index == 1);
  OTTER_ASSERT(virtual_machine->stack[0]->type == OTTER_OBJECT_TYPE_BOOL);

  OTTER_TEST_END(otter_vm_free(virtual_machine); free_bytecode(bytecode););
}

/* Test: OP_JUMP_IF_FALSE does not jump when true */
OTTER_TEST(vm_op_jump_if_false_no_jump) {
  uint8_t instructions[] = {OP_TRUE,                /* push true */
                            OP_JUMP_IF_FALSE, 0, 1, /* should not jump */
                            OP_POP,                 /* should execute */
                            OP_HALT};
  otter_bytecode *bytecode =
      create_bytecode(OTTER_TEST_ALLOCATOR, instructions, sizeof(instructions));

  otter_logger logger = {.vtable = NULL};
  otter_vm *virtual_machine =
      otter_vm_create(OTTER_TEST_ALLOCATOR, bytecode, &logger);

  otter_vm_run(virtual_machine);

  /* If jump didn't happen, POP executed and stack is empty */
  OTTER_ASSERT(virtual_machine->stack_index == 0);

  OTTER_TEST_END(otter_vm_free(virtual_machine); free_bytecode(bytecode););
}
