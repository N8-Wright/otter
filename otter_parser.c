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
#include "otter_parser.h"
otter_parser *otter_parser_create(otter_allocator *allocator,
                                  otter_token **tokens, size_t tokens_length) {
  otter_parser *parser = otter_malloc(allocator, sizeof(*parser));
  if (parser == NULL) {
    return NULL;
  }

  parser->allocator = allocator;
  parser->tokens = tokens;
  parser->tokens_index = 0;
  parser->tokens_length = tokens_length;
  return parser;
}

void otter_parser_free(otter_parser *parser) {
  for (size_t i = 0; i < parser->tokens_length; i++) {
    otter_token_free(parser->allocator, parser->tokens[i]);
  }

  otter_free(parser->allocator, parser->tokens);
  otter_free(parser->allocator, parser);
}
