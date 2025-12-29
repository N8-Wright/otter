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

#include "otter_lexer.h"
#include "otter_node.h"
#include "otter_parser.h"
#include "otter_test.h"

static otter_node **otter_parse_string(otter_allocator *allocator,
                                       const char *str, size_t *nodes_length) {
  OTTER_CLEANUP(otter_logger_free_p)
  otter_logger *logger = otter_logger_create(allocator, OTTER_LOG_LEVEL_DEBUG);
  otter_logger_add_sink(logger, otter_logger_console_sink);

  OTTER_CLEANUP(otter_lexer_free_p)
  otter_lexer *lexer = otter_lexer_create(allocator, str);

  size_t tokens_length = 0;
  otter_token **tokens = otter_lexer_tokenize(lexer, &tokens_length);
  if (tokens == NULL) {
    return NULL;
  }

  OTTER_CLEANUP(otter_parser_free_p)
  otter_parser *parser =
      otter_parser_create(allocator, tokens, tokens_length, logger);
  size_t statements_length = 0;
  otter_node **statements = otter_parser_parse(parser, &statements_length);
  if (statements == NULL) {
    return NULL;
  }

  *nodes_length = statements_length;
  return statements;
}

OTTER_TEST(left_paren) {
  size_t nodes_length = 0;
  otter_node **nodes =
      otter_parse_string(OTTER_TEST_ALLOCATOR, "(", &nodes_length);
  OTTER_ASSERT(nodes_length == 0);
  OTTER_ASSERT(nodes == NULL);

  OTTER_TEST_END();
}

OTTER_TEST(right_paren) {
  size_t nodes_length = 0;
  otter_node **nodes =
      otter_parse_string(OTTER_TEST_ALLOCATOR, ")", &nodes_length);
  OTTER_ASSERT(nodes_length == 0);
  OTTER_ASSERT(nodes == NULL);

  OTTER_TEST_END();
}

OTTER_TEST(empty_parens) {
  size_t nodes_length = 0;
  otter_node **nodes =
      otter_parse_string(OTTER_TEST_ALLOCATOR, "()", &nodes_length);
  OTTER_ASSERT(nodes_length == 0);
  OTTER_ASSERT(nodes == NULL);

  OTTER_TEST_END();
}

OTTER_TEST(left_bracket) {
  size_t nodes_length = 0;
  otter_node **nodes =
      otter_parse_string(OTTER_TEST_ALLOCATOR, "{", &nodes_length);
  OTTER_ASSERT(nodes_length == 0);
  OTTER_ASSERT(nodes == NULL);

  OTTER_TEST_END();
}

OTTER_TEST(right_bracket) {
  size_t nodes_length = 0;
  otter_node **nodes =
      otter_parse_string(OTTER_TEST_ALLOCATOR, "}", &nodes_length);
  OTTER_ASSERT(nodes_length == 0);
  OTTER_ASSERT(nodes == NULL);

  OTTER_TEST_END();
}

OTTER_TEST(empty_brackets) {
  size_t nodes_length = 0;
  otter_node **nodes =
      otter_parse_string(OTTER_TEST_ALLOCATOR, "{}", &nodes_length);
  OTTER_ASSERT(nodes_length == 0);
  OTTER_ASSERT(nodes == NULL);

  OTTER_TEST_END();
}
