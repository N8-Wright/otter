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

OTTER_TEST(vm_op_add_integers) {
  const int left_operand = 10;
  const int right_operand = 32;
  const int expected_sum = 42;

  uint8_t instructions[] = {OP_CONSTANT, 0, OP_CONSTANT, 1, OP_ADD, OP_HALT};
  otter_bytecode *bytecode =
      create_bytecode(OTTER_TEST_ALLOCATOR, instructions, sizeof(instructions));

  /* Create constants */
  otter_object_integer *const1 =
      otter_malloc(OTTER_TEST_ALLOCATOR, sizeof(*const1));
  const1->base.type = OTTER_OBJECT_TYPE_INTEGER;
  const1->value = left_operand;

  otter_object_integer *const2 =
      otter_malloc(OTTER_TEST_ALLOCATOR, sizeof(*const2));
  const2->base.type = OTTER_OBJECT_TYPE_INTEGER;
  const2->value = right_operand;

  bytecode_add_constant(bytecode, (otter_object *)const1);
  bytecode_add_constant(bytecode, (otter_object *)const2);

  otter_logger logger = {.vtable = NULL};
  otter_vm *virtual_machine =
      otter_vm_create(OTTER_TEST_ALLOCATOR, bytecode, &logger);

  otter_vm_run(virtual_machine);

  /* Check result */
  OTTER_ASSERT(virtual_machine->stack_index == 1);
  OTTER_ASSERT(virtual_machine->stack[0]->type == OTTER_OBJECT_TYPE_INTEGER);
  OTTER_ASSERT(((otter_object_integer *)virtual_machine->stack[0])->value ==
               expected_sum);

  OTTER_TEST_END(otter_free(OTTER_TEST_ALLOCATOR, const1);
                 otter_free(OTTER_TEST_ALLOCATOR, const2);
                 otter_vm_free(virtual_machine); free_bytecode(bytecode););
}

OTTER_TEST(vm_op_subtract_integers) {
  const int minuend = 50;
  const int subtrahend = 8;
  const int expected_difference = 42;

  uint8_t instructions[] = {OP_CONSTANT, 0,           OP_CONSTANT,
                            1,           OP_SUBTRACT, OP_HALT};
  otter_bytecode *bytecode =
      create_bytecode(OTTER_TEST_ALLOCATOR, instructions, sizeof(instructions));

  otter_object_integer *const1 =
      otter_malloc(OTTER_TEST_ALLOCATOR, sizeof(*const1));
  const1->base.type = OTTER_OBJECT_TYPE_INTEGER;
  const1->value = minuend;

  otter_object_integer *const2 =
      otter_malloc(OTTER_TEST_ALLOCATOR, sizeof(*const2));
  const2->base.type = OTTER_OBJECT_TYPE_INTEGER;
  const2->value = subtrahend;

  bytecode_add_constant(bytecode, (otter_object *)const1);
  bytecode_add_constant(bytecode, (otter_object *)const2);

  otter_logger logger = {.vtable = NULL};
  otter_vm *virtual_machine =
      otter_vm_create(OTTER_TEST_ALLOCATOR, bytecode, &logger);

  otter_vm_run(virtual_machine);

  OTTER_ASSERT(virtual_machine->stack_index == 1);
  OTTER_ASSERT(((otter_object_integer *)virtual_machine->stack[0])->value ==
               expected_difference);

  OTTER_TEST_END(otter_free(OTTER_TEST_ALLOCATOR, const1);
                 otter_free(OTTER_TEST_ALLOCATOR, const2);
                 otter_vm_free(virtual_machine); free_bytecode(bytecode););
}

OTTER_TEST(vm_op_multiply_integers) {
  const int multiplicand = 6;
  const int multiplier = 7;
  const int expected_product = 42;

  uint8_t instructions[] = {OP_CONSTANT, 0,           OP_CONSTANT,
                            1,           OP_MULTIPLY, OP_HALT};
  otter_bytecode *bytecode =
      create_bytecode(OTTER_TEST_ALLOCATOR, instructions, sizeof(instructions));

  otter_object_integer *const1 =
      otter_malloc(OTTER_TEST_ALLOCATOR, sizeof(*const1));
  const1->base.type = OTTER_OBJECT_TYPE_INTEGER;
  const1->value = multiplicand;

  otter_object_integer *const2 =
      otter_malloc(OTTER_TEST_ALLOCATOR, sizeof(*const2));
  const2->base.type = OTTER_OBJECT_TYPE_INTEGER;
  const2->value = multiplier;

  bytecode_add_constant(bytecode, (otter_object *)const1);
  bytecode_add_constant(bytecode, (otter_object *)const2);

  otter_logger logger = {.vtable = NULL};
  otter_vm *virtual_machine =
      otter_vm_create(OTTER_TEST_ALLOCATOR, bytecode, &logger);

  otter_vm_run(virtual_machine);

  OTTER_ASSERT(virtual_machine->stack_index == 1);
  OTTER_ASSERT(((otter_object_integer *)virtual_machine->stack[0])->value ==
               expected_product);

  OTTER_TEST_END(otter_free(OTTER_TEST_ALLOCATOR, const1);
                 otter_free(OTTER_TEST_ALLOCATOR, const2);
                 otter_vm_free(virtual_machine); free_bytecode(bytecode););
}

OTTER_TEST(vm_op_divide_integers) {
  const int dividend = 84;
  const int divisor = 2;
  const int expected_quotient = 42;

  uint8_t instructions[] = {OP_CONSTANT, 0, OP_CONSTANT, 1, OP_DIVIDE, OP_HALT};
  otter_bytecode *bytecode =
      create_bytecode(OTTER_TEST_ALLOCATOR, instructions, sizeof(instructions));

  otter_object_integer *const1 =
      otter_malloc(OTTER_TEST_ALLOCATOR, sizeof(*const1));
  const1->base.type = OTTER_OBJECT_TYPE_INTEGER;
  const1->value = dividend;

  otter_object_integer *const2 =
      otter_malloc(OTTER_TEST_ALLOCATOR, sizeof(*const2));
  const2->base.type = OTTER_OBJECT_TYPE_INTEGER;
  const2->value = divisor;

  bytecode_add_constant(bytecode, (otter_object *)const1);
  bytecode_add_constant(bytecode, (otter_object *)const2);

  otter_logger logger = {.vtable = NULL};
  otter_vm *virtual_machine =
      otter_vm_create(OTTER_TEST_ALLOCATOR, bytecode, &logger);

  otter_vm_run(virtual_machine);

  OTTER_ASSERT(virtual_machine->stack_index == 1);
  OTTER_ASSERT(((otter_object_integer *)virtual_machine->stack[0])->value ==
               expected_quotient);

  OTTER_TEST_END(otter_free(OTTER_TEST_ALLOCATOR, const1);
                 otter_free(OTTER_TEST_ALLOCATOR, const2);
                 otter_vm_free(virtual_machine); free_bytecode(bytecode););
}

OTTER_TEST(vm_op_negate_integer) {
  const int positive_value = 42;
  const int expected_negated = -42;

  uint8_t instructions[] = {OP_CONSTANT, 0, OP_NEGATE, OP_HALT};
  otter_bytecode *bytecode =
      create_bytecode(OTTER_TEST_ALLOCATOR, instructions, sizeof(instructions));

  otter_object_integer *const1 =
      otter_malloc(OTTER_TEST_ALLOCATOR, sizeof(*const1));
  const1->base.type = OTTER_OBJECT_TYPE_INTEGER;
  const1->value = positive_value;

  bytecode_add_constant(bytecode, (otter_object *)const1);

  otter_logger logger = {.vtable = NULL};
  otter_vm *virtual_machine =
      otter_vm_create(OTTER_TEST_ALLOCATOR, bytecode, &logger);

  otter_vm_run(virtual_machine);

  OTTER_ASSERT(virtual_machine->stack_index == 1);
  OTTER_ASSERT(((otter_object_integer *)virtual_machine->stack[0])->value ==
               expected_negated);

  OTTER_TEST_END(otter_free(OTTER_TEST_ALLOCATOR, const1);
                 otter_vm_free(virtual_machine); free_bytecode(bytecode););
}
