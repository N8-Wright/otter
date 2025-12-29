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

#define otter_parse_string(str, nodes_length)                                  \
  ({                                                                           \
    OTTER_CLEANUP(otter_logger_free_p)                                         \
    otter_logger *otter_logger_ =                                              \
        otter_logger_create(OTTER_TEST_ALLOCATOR, OTTER_LOG_LEVEL_DEBUG);      \
    otter_logger_add_sink(otter_logger_, otter_logger_console_sink);           \
                                                                               \
    OTTER_CLEANUP(otter_lexer_free_p)                                          \
    otter_lexer *otter_lexer_ = otter_lexer_create(OTTER_TEST_ALLOCATOR, str); \
                                                                               \
    size_t otter_tokens_length_ = 0;                                           \
    otter_token **otter_tokens_ =                                              \
        otter_lexer_tokenize(otter_lexer_, &otter_tokens_length_);             \
    OTTER_ASSERT(otter_tokens_ != NULL);                                       \
                                                                               \
    OTTER_CLEANUP(otter_parser_free_p)                                         \
    otter_parser *otter_parser_ =                                              \
        otter_parser_create(OTTER_TEST_ALLOCATOR, otter_tokens_,               \
                            otter_tokens_length_, otter_logger_);              \
    size_t otter_statements_length_ = 0;                                       \
    otter_node **otter_statements_ =                                           \
        otter_parser_parse(otter_parser_, &otter_statements_length_);          \
    *nodes_length = otter_statements_length_;                                  \
    otter_statements_;                                                         \
  })

OTTER_TEST(left_paren) {
  size_t nodes_length = 0;
  otter_node **nodes = otter_parse_string("(", &nodes_length);
  OTTER_ASSERT(nodes_length == 0);
  OTTER_ASSERT(nodes == NULL);

  OTTER_TEST_END();
}

OTTER_TEST(right_paren) {
  size_t nodes_length = 0;
  otter_node **nodes = otter_parse_string(")", &nodes_length);
  OTTER_ASSERT(nodes_length == 0);
  OTTER_ASSERT(nodes == NULL);

  OTTER_TEST_END();
}

OTTER_TEST(empty_parens) {
  size_t nodes_length = 0;
  otter_node **nodes = otter_parse_string("()", &nodes_length);
  OTTER_ASSERT(nodes_length == 0);
  OTTER_ASSERT(nodes == NULL);

  OTTER_TEST_END();
}

OTTER_TEST(left_bracket) {
  size_t nodes_length = 0;
  otter_node **nodes = otter_parse_string("{", &nodes_length);
  OTTER_ASSERT(nodes_length == 0);
  OTTER_ASSERT(nodes == NULL);

  OTTER_TEST_END();
}

OTTER_TEST(right_bracket) {
  size_t nodes_length = 0;
  otter_node **nodes = otter_parse_string("}", &nodes_length);
  OTTER_ASSERT(nodes_length == 0);
  OTTER_ASSERT(nodes == NULL);

  OTTER_TEST_END();
}

OTTER_TEST(empty_brackets) {
  size_t nodes_length = 0;
  otter_node **nodes = otter_parse_string("{}", &nodes_length);
  OTTER_ASSERT(nodes_length == 0);
  OTTER_ASSERT(nodes == NULL);

  OTTER_TEST_END();
}
