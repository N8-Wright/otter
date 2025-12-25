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
#ifndef OTTER_NODE_H_
#define OTTER_NODE_H_
#include "otter_allocator.h"
#include "otter_array.h"
typedef enum otter_node_type {
  OTTER_NODE_IDENTIFIER,
  OTTER_NODE_INTEGER,

  OTTER_NODE_STATEMENT_ASSIGNMENT,
  OTTER_NODE_STATEMENT_FOR,

  OTTER_NODE_EXPRESSION_ADD,
  OTTER_NODE_EXPRESSION_INCREMENT,
} otter_node_type;

typedef struct otter_node {
  otter_node_type type;
} otter_node;

typedef struct otter_node_identifier {
  otter_node base;
  char *value;
} otter_node_identifier;

typedef struct otter_node_integer {
  otter_node base;
  int value;
} otter_node_integer;

typedef struct otter_node_add {
  otter_node base;
  otter_node *left;
  otter_node *right;
} otter_node_add;

typedef struct otter_node_increment {
  otter_node base;
  otter_node *value;
} otter_node_increment;

typedef struct otter_node_assignment {
  otter_node base;
  otter_node_identifier *variable;
  otter_node *value_expr;
} otter_node_assignment;

typedef struct otter_node_for {
  otter_node base;
  otter_node_assignment *assignment;
  otter_node *condition;
  otter_node *iteration;

  OTTER_ARRAY_DECLARE(otter_node *, statements);
} otter_node_for;

void otter_node_free(otter_allocator *allocator, otter_node *node);
#endif /* OTTER_NODE_H_ */
