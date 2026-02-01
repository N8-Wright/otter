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
#ifndef OTTER_BYTECODE_H_
#define OTTER_BYTECODE_H_
#include "allocator.h"
#include "array.h"
#include "inc.h"
#include "logger.h"
#include "object.h"

typedef enum otter_opcode {
  /* Constants */
  OP_CONSTANT, /* Load constant from constant pool [1 byte index] */
  OP_NIL,      /* Push nil */
  OP_TRUE,     /* Push true */
  OP_FALSE,    /* Push false */

  /* Stack manipulation */
  OP_POP,  /* Pop top of stack */
  OP_DUP,  /* Duplicate top of stack */
  OP_SWAP, /* Swap top two stack values */

  /* Arithmetic operations */
  OP_ADD,      /* Add top two values */
  OP_SUBTRACT, /* Subtract top from second */
  OP_MULTIPLY, /* Multiply top two values */
  OP_DIVIDE,   /* Divide second by top */
  OP_MODULO,   /* Modulo second by top */
  OP_NEGATE,   /* Negate top value */

  /* Comparison operations */
  OP_EQUAL,         /* Check equality */
  OP_NOT_EQUAL,     /* Check inequality */
  OP_LESS,          /* Less than */
  OP_LESS_EQUAL,    /* Less than or equal */
  OP_GREATER,       /* Greater than */
  OP_GREATER_EQUAL, /* Greater than or equal */

  /* Logical operations */
  OP_NOT, /* Logical not */
  OP_AND, /* Logical and */
  OP_OR,  /* Logical or */

  /* Variables */
  OP_GET_LOCAL,     /* Get local variable [1 byte index] */
  OP_SET_LOCAL,     /* Set local variable [1 byte index] */
  OP_GET_GLOBAL,    /* Get global variable [1 byte index] */
  OP_SET_GLOBAL,    /* Set global variable [1 byte index] */
  OP_DEFINE_GLOBAL, /* Define global variable [1 byte index] */

  /* Control flow */
  OP_JUMP,          /* Unconditional jump [2 byte offset] */
  OP_JUMP_IF_FALSE, /* Jump if top is false [2 byte offset] */
  OP_LOOP,          /* Jump backward [2 byte offset] */
  OP_CALL,          /* Call function [1 byte arg count] */
  OP_RETURN,        /* Return from function */

  /* Other */
  OP_PRINT, /* Print top of stack */
  OP_HALT,  /* Halt execution */
} otter_opcode;

typedef struct otter_bytecode {
  otter_allocator *allocator;
  OTTER_ARRAY_DECLARE(otter_object *, constants);
  const char *instructions;
} otter_bytecode;

otter_bytecode *otter_bytecode_create(otter_allocator *allocator,
                                      const char *src, size_t source_length,
                                      otter_logger *logger);
void otter_bytecode_free(otter_bytecode *bytecode);
OTTER_DECLARE_TRIVIAL_CLEANUP_FUNC(otter_bytecode *, otter_bytecode_free);
#endif /* OTTER_BYTECODE_H_ */
