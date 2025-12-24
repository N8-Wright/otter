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
#include "otter_array.h"
#include "otter_cstring.h"
#include "otter_node.h"
#include "otter_parser.h"
#include "otter_test.h"
#include <string.h>

typedef struct token_array {
  OTTER_ARRAY_DECLARE(otter_token *, value);
} token_array;

#define CREATE_BASIC_TOKEN(allocator, token_type)                              \
  ({                                                                           \
    otter_token *token = otter_malloc(allocator, sizeof(*token));              \
    OTTER_ASSERT(token != NULL);                                               \
    token->type = token_type;                                                  \
    token;                                                                     \
  })

#define CREATE_IDENT_TOKEN(allocator, ident)                                   \
  ({                                                                           \
    otter_token_identifier *token = otter_malloc(allocator, sizeof(*token));   \
    OTTER_ASSERT(token != NULL);                                               \
    token->base.type = OTTER_TOKEN_IDENTIFIER;                                 \
    token->value = otter_strdup(allocator, ident);                             \
    OTTER_ASSERT(token->value != NULL);                                        \
    (otter_token *)token;                                                      \
  })

#define CREATE_INTEGER_TOKEN(allocator, val)                                   \
  ({                                                                           \
    otter_token_integer *token = otter_malloc(allocator, sizeof(*token));      \
    OTTER_ASSERT(token != NULL);                                               \
    token->base.type = OTTER_TOKEN_INTEGER;                                    \
    token->value = val;                                                        \
    (otter_token *)token;                                                      \
  })

#define APPEND_BASIC_TOKEN(arr, token_type)                                    \
  OTTER_ARRAY_APPEND(arr, value, OTTER_TEST_ALLOCATOR,                         \
                     CREATE_BASIC_TOKEN(OTTER_TEST_ALLOCATOR, token_type))

#define APPEND_IDENTIFIER(arr, ident)                                          \
  OTTER_ARRAY_APPEND(arr, value, OTTER_TEST_ALLOCATOR,                         \
                     CREATE_IDENT_TOKEN(OTTER_TEST_ALLOCATOR, ident))

#define APPEND_INTEGER(arr, val)                                               \
  OTTER_ARRAY_APPEND(arr, value, OTTER_TEST_ALLOCATOR,                         \
                     CREATE_INTEGER_TOKEN(OTTER_TEST_ALLOCATOR, val))

OTTER_TEST(parse_addition_expression) {
  token_array tokens = {};
  OTTER_ARRAY_INIT(&tokens, value, OTTER_TEST_ALLOCATOR);
  APPEND_INTEGER(&tokens, 8);
  APPEND_BASIC_TOKEN(&tokens, OTTER_TOKEN_PLUS);
  APPEND_INTEGER(&tokens, 10);
  APPEND_BASIC_TOKEN(&tokens, OTTER_TOKEN_SEMICOLON);

  OTTER_CLEANUP(otter_parser_free_p)
  otter_parser *parser = otter_parser_create(OTTER_TEST_ALLOCATOR, tokens.value,
                                             tokens.value_length);

  size_t statements_length = 0;
  otter_node **statements = otter_parser_parse(parser, &statements_length);
  OTTER_ASSERT(statements != NULL);
  OTTER_ASSERT(statements_length == 1);
  OTTER_ASSERT(statements[0]->type == OTTER_NODE_EXPRESSION_ADD);
  otter_node_add *addition = (otter_node_add *)statements[0];
  OTTER_ASSERT(addition->left->type == OTTER_NODE_INTEGER);
  OTTER_ASSERT(addition->right->type == OTTER_NODE_INTEGER);
  otter_node_integer *left = (otter_node_integer *)addition->left;
  otter_node_integer *right = (otter_node_integer *)addition->right;
  OTTER_ASSERT(left->value == 8);
  OTTER_ASSERT(right->value == 10);

  OTTER_TEST_END(if (statements != NULL) {
    otter_node_free(OTTER_TEST_ALLOCATOR, statements[0]);
    otter_free(OTTER_TEST_ALLOCATOR, statements);
  });
}

OTTER_TEST(parse_assignment) {
  token_array tokens = {};
  OTTER_ARRAY_INIT(&tokens, value, OTTER_TEST_ALLOCATOR);
  APPEND_BASIC_TOKEN(&tokens, OTTER_TOKEN_VAR);
  APPEND_IDENTIFIER(&tokens, "foobar");
  APPEND_BASIC_TOKEN(&tokens, OTTER_TOKEN_ASSIGNMENT);
  APPEND_INTEGER(&tokens, 2);
  APPEND_BASIC_TOKEN(&tokens, OTTER_TOKEN_SEMICOLON);

  OTTER_CLEANUP(otter_parser_free_p)
  otter_parser *parser = otter_parser_create(OTTER_TEST_ALLOCATOR, tokens.value,
                                             tokens.value_length);

  size_t statements_length = 0;
  otter_node **statements = otter_parser_parse(parser, &statements_length);
  OTTER_ASSERT(statements != NULL);
  OTTER_ASSERT(statements_length == 1);
  OTTER_ASSERT(statements[0]->type == OTTER_NODE_STATEMENT_ASSIGNMENT);
  otter_node_assignment *assignment = (otter_node_assignment *)statements[0];
  OTTER_ASSERT(0 == strcmp(assignment->variable->value, "foobar"));
  OTTER_ASSERT(assignment->value_expr->type == OTTER_NODE_INTEGER);
  otter_node_integer *value = (otter_node_integer *)assignment->value_expr;
  OTTER_ASSERT(value->value == 2);

  OTTER_TEST_END(if (statements != NULL) {
    otter_node_free(OTTER_TEST_ALLOCATOR, statements[0]);
    otter_free(OTTER_TEST_ALLOCATOR, statements);
  });
}
