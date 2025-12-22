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
#include "otter_node.h"
void otter_node_free(otter_allocator *allocator, otter_node *node) {
  if (allocator == NULL || node == NULL) {
    return;
  }

  switch (node->type) {
  case OTTER_NODE_IDENTIFIER: {
    otter_free(allocator, ((otter_node_identifier *)node)->value);
  } break;
  case OTTER_NODE_STATEMENT_ASSIGNMENT: {
    otter_node_assignment *assignment = (otter_node_assignment *)node;
    otter_node_free(allocator, (otter_node *)assignment->variable);
    otter_node_free(allocator, assignment->value_expr);
  } break;
  default:
    break;
  }

  otter_free(allocator, node);
}
