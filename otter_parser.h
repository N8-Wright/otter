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
#ifndef OTTER_PARSER_H_
#define OTTER_PARSER_H_
#include "otter_allocator.h"
#include "otter_inc.h"
#include "otter_node.h"
#include "otter_token.h"

typedef struct otter_parser {
  otter_allocator *allocator;
  otter_token **tokens;
  size_t tokens_index;
  size_t tokens_length;
} otter_parser;

otter_parser *otter_parser_create(otter_allocator *allocator,
                                  otter_token **tokens, size_t tokens_length);
void otter_parser_free(otter_parser *parser);
OTTER_DEFINE_TRIVIAL_CLEANUP_FUNC(otter_parser *, otter_parser_free);
otter_node **otter_parser_parse(otter_parser *, size_t *nodes_length);

#endif /* OTTER_PARSER_H_ */
