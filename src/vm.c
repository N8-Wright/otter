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
#include "otter/vm.h"
#include <stdint.h>

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
  virtual_machine->objects = NULL;

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
  /* Free all objects in the linked list */
  otter_object *current = virtual_machine->objects;
  while (current != NULL) {
    otter_object *next = current->next;
    otter_free(virtual_machine->allocator, current);
    current = next;
  }

  otter_free(virtual_machine->allocator, virtual_machine->stack);
  otter_free(virtual_machine->allocator, virtual_machine);
}

OTTER_DEFINE_TRIVIAL_CLEANUP_FUNC(otter_vm *, otter_vm_free);

/* Allocate an object and add it to the VM's object list */
static inline otter_object *vm_allocate_object(otter_vm *virtual_machine,
                                               size_t size) {
  otter_object *obj = otter_malloc(virtual_machine->allocator, size);
  obj->next = virtual_machine->objects;
  virtual_machine->objects = obj;
  return obj;
}

/* Stack operations */
static inline void vm_push(otter_vm *virtual_machine, otter_object *obj) {
  virtual_machine->stack[virtual_machine->stack_index++] = obj;
}

static inline otter_object *vm_pop(otter_vm *virtual_machine) {
  return virtual_machine->stack[--virtual_machine->stack_index];
}

static inline otter_object *vm_peek(otter_vm *virtual_machine,
                                    size_t distance) {
  return virtual_machine->stack[virtual_machine->stack_index - 1 - distance];
}

/* Helper to read a byte from bytecode */
static inline uint8_t read_byte(const uint8_t **instr_ptr) {
  return *(*instr_ptr)++;
}

/* Helper to read a 16-bit value from bytecode */
static inline uint16_t read_short(const uint8_t **instr_ptr) {
  const uint16_t bits_per_byte = 8;
  uint16_t value =
      (uint16_t)((*instr_ptr)[0] << bits_per_byte) | (*instr_ptr)[1];
  *instr_ptr += 2;
  return value;
}

/* Arithmetic operations */
static otter_object *vm_add(otter_vm *virtual_machine, otter_object *lhs,
                            otter_object *rhs) {
  if (lhs->type == OTTER_OBJECT_TYPE_INTEGER &&
      rhs->type == OTTER_OBJECT_TYPE_INTEGER) {
    otter_object_integer *result = (otter_object_integer *)vm_allocate_object(
        virtual_machine, sizeof(*result));
    result->base.type = OTTER_OBJECT_TYPE_INTEGER;
    result->value = ((otter_object_integer *)lhs)->value +
                    ((otter_object_integer *)rhs)->value;
    return (otter_object *)result;
  }
  return NULL;
}

static otter_object *vm_subtract(otter_vm *virtual_machine, otter_object *lhs,
                                 otter_object *rhs) {
  if (lhs->type == OTTER_OBJECT_TYPE_INTEGER &&
      rhs->type == OTTER_OBJECT_TYPE_INTEGER) {
    otter_object_integer *result = (otter_object_integer *)vm_allocate_object(
        virtual_machine, sizeof(*result));
    result->base.type = OTTER_OBJECT_TYPE_INTEGER;
    result->value = ((otter_object_integer *)lhs)->value -
                    ((otter_object_integer *)rhs)->value;
    return (otter_object *)result;
  }
  return NULL;
}

static otter_object *vm_multiply(otter_vm *virtual_machine, otter_object *lhs,
                                 otter_object *rhs) {
  if (lhs->type == OTTER_OBJECT_TYPE_INTEGER &&
      rhs->type == OTTER_OBJECT_TYPE_INTEGER) {
    otter_object_integer *result = (otter_object_integer *)vm_allocate_object(
        virtual_machine, sizeof(*result));
    result->base.type = OTTER_OBJECT_TYPE_INTEGER;
    result->value = ((otter_object_integer *)lhs)->value *
                    ((otter_object_integer *)rhs)->value;
    return (otter_object *)result;
  }
  return NULL;
}

static otter_object *vm_divide(otter_vm *virtual_machine, otter_object *lhs,
                               otter_object *rhs) {
  if (lhs->type == OTTER_OBJECT_TYPE_INTEGER &&
      rhs->type == OTTER_OBJECT_TYPE_INTEGER) {
    int divisor = ((otter_object_integer *)rhs)->value;
    if (divisor == 0) {
      otter_log_error(virtual_machine->logger, "Division by zero");
      return NULL;
    }
    otter_object_integer *result = (otter_object_integer *)vm_allocate_object(
        virtual_machine, sizeof(*result));
    result->base.type = OTTER_OBJECT_TYPE_INTEGER;
    result->value = ((otter_object_integer *)lhs)->value / divisor;
    return (otter_object *)result;
  }
  return NULL;
}

/* Comparison operations */
static otter_object *vm_compare_equal(otter_vm *virtual_machine,
                                      otter_object *lhs, otter_object *rhs) {
  otter_object_bool *result =
      (otter_object_bool *)vm_allocate_object(virtual_machine, sizeof(*result));
  result->base.type = OTTER_OBJECT_TYPE_BOOL;
  result->value = 0;

  if (lhs->type == rhs->type) {
    if (lhs->type == OTTER_OBJECT_TYPE_INTEGER) {
      result->value = ((otter_object_integer *)lhs)->value ==
                      ((otter_object_integer *)rhs)->value;
    } else if (lhs->type == OTTER_OBJECT_TYPE_BOOL) {
      result->value = ((otter_object_bool *)lhs)->value ==
                      ((otter_object_bool *)rhs)->value;
    }
  }

  return (otter_object *)result;
}

static otter_object *vm_compare_less(otter_vm *virtual_machine,
                                     otter_object *lhs, otter_object *rhs) {
  otter_object_bool *result =
      (otter_object_bool *)vm_allocate_object(virtual_machine, sizeof(*result));
  result->base.type = OTTER_OBJECT_TYPE_BOOL;

  if (lhs->type == OTTER_OBJECT_TYPE_INTEGER &&
      rhs->type == OTTER_OBJECT_TYPE_INTEGER) {
    result->value = ((otter_object_integer *)lhs)->value <
                    ((otter_object_integer *)rhs)->value;
  } else {
    result->value = 0;
  }

  return (otter_object *)result;
}

static otter_object *vm_compare_greater(otter_vm *virtual_machine,
                                        otter_object *lhs, otter_object *rhs) {
  otter_object_bool *result =
      (otter_object_bool *)vm_allocate_object(virtual_machine, sizeof(*result));
  result->base.type = OTTER_OBJECT_TYPE_BOOL;

  if (lhs->type == OTTER_OBJECT_TYPE_INTEGER &&
      rhs->type == OTTER_OBJECT_TYPE_INTEGER) {
    result->value = ((otter_object_integer *)lhs)->value >
                    ((otter_object_integer *)rhs)->value;
  } else {
    result->value = 0;
  }

  return (otter_object *)result;
}

/* Check if object is falsey (nil or false) */
static int is_falsey(otter_object *obj) {
  if (obj->type == OTTER_OBJECT_TYPE_NIL) {
    return 1;
  }
  if (obj->type == OTTER_OBJECT_TYPE_BOOL) {
    return !((otter_object_bool *)obj)->value;
  }
  return 0;
}

void otter_vm_run(otter_vm *virtual_machine) {
  const uint8_t *instr_ptr =
      (const uint8_t *)virtual_machine->bytecode->instructions;
  otter_object **globals = NULL;
  size_t globals_capacity = 0;

#define BINARY_OP(op_func)                                                     \
  do {                                                                         \
    otter_object *rhs = vm_pop(virtual_machine);                               \
    otter_object *lhs = vm_pop(virtual_machine);                               \
    otter_object *result = op_func(virtual_machine, lhs, rhs);                 \
    if (result == NULL) {                                                      \
      otter_log_error(virtual_machine->logger,                                 \
                      "Runtime error in binary operation");                    \
      goto vm_cleanup;                                                         \
    }                                                                          \
    vm_push(virtual_machine, result);                                          \
  } while (0)

/* NOLINTBEGIN(bugprone-macro-parentheses) */
#define DISPATCH() goto *dispatch_table[read_byte(&instr_ptr)]
  /* NOLINTEND(bugprone-macro-parentheses) */

  static const void *dispatch_table[] = {
      [OP_CONSTANT] = &&op_constant,
      [OP_NIL] = &&op_nil,
      [OP_TRUE] = &&op_true,
      [OP_FALSE] = &&op_false,
      [OP_POP] = &&op_pop,
      [OP_DUP] = &&op_dup,
      [OP_SWAP] = &&op_swap,
      [OP_ADD] = &&op_add,
      [OP_SUBTRACT] = &&op_subtract,
      [OP_MULTIPLY] = &&op_multiply,
      [OP_DIVIDE] = &&op_divide,
      [OP_MODULO] = &&op_modulo,
      [OP_NEGATE] = &&op_negate,
      [OP_EQUAL] = &&op_equal,
      [OP_NOT_EQUAL] = &&op_not_equal,
      [OP_LESS] = &&op_less,
      [OP_LESS_EQUAL] = &&op_less_equal,
      [OP_GREATER] = &&op_greater,
      [OP_GREATER_EQUAL] = &&op_greater_equal,
      [OP_NOT] = &&op_not,
      [OP_AND] = &&op_and,
      [OP_OR] = &&op_or,
      [OP_GET_LOCAL] = &&op_get_local,
      [OP_SET_LOCAL] = &&op_set_local,
      [OP_GET_GLOBAL] = &&op_get_global,
      [OP_SET_GLOBAL] = &&op_set_global,
      [OP_DEFINE_GLOBAL] = &&op_define_global,
      [OP_JUMP] = &&op_jump,
      [OP_JUMP_IF_FALSE] = &&op_jump_if_false,
      [OP_LOOP] = &&op_loop,
      [OP_CALL] = &&op_call,
      [OP_RETURN] = &&op_return,
      [OP_PRINT] = &&op_print,
      [OP_HALT] = &&op_halt,
  };

  DISPATCH();

op_constant: {
  uint8_t constant_index = read_byte(&instr_ptr);
  otter_object *constant = virtual_machine->bytecode->constants[constant_index];
  vm_push(virtual_machine, constant);
  DISPATCH();
}

op_nil: {
  otter_object *nil = vm_allocate_object(virtual_machine, sizeof(*nil));
  nil->type = OTTER_OBJECT_TYPE_NIL;
  vm_push(virtual_machine, nil);
  DISPATCH();
}

op_true: {
  otter_object_bool *true_obj = (otter_object_bool *)vm_allocate_object(
      virtual_machine, sizeof(*true_obj));
  true_obj->base.type = OTTER_OBJECT_TYPE_BOOL;
  true_obj->value = 1;
  vm_push(virtual_machine, (otter_object *)true_obj);
  DISPATCH();
}

op_false: {
  otter_object_bool *false_obj = (otter_object_bool *)vm_allocate_object(
      virtual_machine, sizeof(*false_obj));
  false_obj->base.type = OTTER_OBJECT_TYPE_BOOL;
  false_obj->value = 0;
  vm_push(virtual_machine, (otter_object *)false_obj);
  DISPATCH();
}

op_pop:
  vm_pop(virtual_machine);
  DISPATCH();

op_dup:
  vm_push(virtual_machine, vm_peek(virtual_machine, 0));
  DISPATCH();

op_swap: {
  otter_object *top = vm_pop(virtual_machine);
  otter_object *second = vm_pop(virtual_machine);
  vm_push(virtual_machine, top);
  vm_push(virtual_machine, second);
  DISPATCH();
}

op_add:
  BINARY_OP(vm_add);
  DISPATCH();

op_subtract:
  BINARY_OP(vm_subtract);
  DISPATCH();

op_multiply:
  BINARY_OP(vm_multiply);
  DISPATCH();

op_divide:
  BINARY_OP(vm_divide);
  DISPATCH();

op_modulo:
  DISPATCH();

op_negate: {
  otter_object *obj = vm_pop(virtual_machine);
  if (obj->type == OTTER_OBJECT_TYPE_INTEGER) {
    otter_object_integer *result = (otter_object_integer *)vm_allocate_object(
        virtual_machine, sizeof(*result));
    result->base.type = OTTER_OBJECT_TYPE_INTEGER;
    result->value = -((otter_object_integer *)obj)->value;
    vm_push(virtual_machine, (otter_object *)result);
  } else {
    otter_log_error(virtual_machine->logger, "Cannot negate non-integer");
    goto vm_cleanup;
  }
  DISPATCH();
}

op_equal:
  BINARY_OP(vm_compare_equal);
  DISPATCH();

op_not_equal: {
  BINARY_OP(vm_compare_equal);
  otter_object_bool *result = (otter_object_bool *)vm_pop(virtual_machine);
  result->value = !result->value;
  vm_push(virtual_machine, (otter_object *)result);
  DISPATCH();
}

op_less:
  BINARY_OP(vm_compare_less);
  DISPATCH();

op_less_equal: {
  BINARY_OP(vm_compare_greater);
  otter_object_bool *result = (otter_object_bool *)vm_pop(virtual_machine);
  result->value = !result->value;
  vm_push(virtual_machine, (otter_object *)result);
  DISPATCH();
}

op_greater:
  BINARY_OP(vm_compare_greater);
  DISPATCH();

op_greater_equal: {
  BINARY_OP(vm_compare_less);
  otter_object_bool *result = (otter_object_bool *)vm_pop(virtual_machine);
  result->value = !result->value;
  vm_push(virtual_machine, (otter_object *)result);
  DISPATCH();
}

op_not: {
  otter_object *obj = vm_pop(virtual_machine);
  otter_object_bool *result =
      (otter_object_bool *)vm_allocate_object(virtual_machine, sizeof(*result));
  result->base.type = OTTER_OBJECT_TYPE_BOOL;
  result->value = is_falsey(obj);
  vm_push(virtual_machine, (otter_object *)result);
  DISPATCH();
}

op_and:
  DISPATCH();

op_or:
  DISPATCH();

op_get_local: {
  uint8_t slot = read_byte(&instr_ptr);
  vm_push(virtual_machine, virtual_machine->stack[slot]);
  DISPATCH();
}

op_set_local: {
  uint8_t slot = read_byte(&instr_ptr);
  virtual_machine->stack[slot] = vm_peek(virtual_machine, 0);
  DISPATCH();
}

op_get_global: {
  uint8_t slot = read_byte(&instr_ptr);
  if (slot >= globals_capacity || globals[slot] == NULL) {
    otter_log_error(virtual_machine->logger, "Undefined global variable");
    goto vm_cleanup;
  }
  vm_push(virtual_machine, globals[slot]);
  DISPATCH();
}

op_set_global: {
  uint8_t slot = read_byte(&instr_ptr);
  if (slot >= globals_capacity) {
    size_t new_capacity = slot + 1;
    otter_object **new_globals =
        otter_realloc(virtual_machine->allocator, globals,
                      sizeof(otter_object *) * new_capacity);
    if (new_globals == NULL) {
      otter_log_critical(virtual_machine->logger, "Failed to allocate globals");
      goto vm_cleanup;
    }
    for (size_t i = globals_capacity; i < new_capacity; i++) {
      new_globals[i] = NULL;
    }
    globals = new_globals;
    globals_capacity = new_capacity;
  }
  globals[slot] = vm_peek(virtual_machine, 0);
  DISPATCH();
}

op_define_global: {
  uint8_t slot = read_byte(&instr_ptr);
  if (slot >= globals_capacity) {
    size_t new_capacity = slot + 1;
    otter_object **new_globals =
        otter_realloc(virtual_machine->allocator, globals,
                      sizeof(otter_object *) * new_capacity);
    if (new_globals == NULL) {
      otter_log_critical(virtual_machine->logger, "Failed to allocate globals");
      goto vm_cleanup;
    }
    for (size_t i = globals_capacity; i < new_capacity; i++) {
      new_globals[i] = NULL;
    }
    globals = new_globals;
    globals_capacity = new_capacity;
  }
  globals[slot] = vm_pop(virtual_machine);
  DISPATCH();
}

op_jump: {
  uint16_t offset = read_short(&instr_ptr);
  instr_ptr += offset;
  DISPATCH();
}

op_jump_if_false: {
  uint16_t offset = read_short(&instr_ptr);
  if (is_falsey(vm_peek(virtual_machine, 0))) {
    instr_ptr += offset;
  }
  DISPATCH();
}

op_loop: {
  uint16_t offset = read_short(&instr_ptr);
  instr_ptr -= offset;
  DISPATCH();
}

op_call:
  DISPATCH();

op_return:
  DISPATCH();

op_print: {
  otter_object *obj = vm_peek(virtual_machine, 0);
  if (obj->type == OTTER_OBJECT_TYPE_INTEGER) {
    otter_log_info(virtual_machine->logger, "%d",
                   ((otter_object_integer *)obj)->value);
  } else if (obj->type == OTTER_OBJECT_TYPE_BOOL) {
    otter_log_info(virtual_machine->logger, "%s",
                   ((otter_object_bool *)obj)->value ? "true" : "false");
  } else if (obj->type == OTTER_OBJECT_TYPE_NIL) {
    otter_log_info(virtual_machine->logger, "nil");
  }
  DISPATCH();
}

op_halt:
  goto vm_cleanup;

vm_cleanup:
  if (globals != NULL) {
    otter_free(virtual_machine->allocator, globals);
  }

#undef BINARY_OP
#undef DISPATCH
}
