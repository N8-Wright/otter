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
#include "otter_array.h"
#include "otter_cstring.h"

static otter_parser_precedence otter_precedence_table[OTTER_TOKEN_COUNT_] = {
    [OTTER_TOKEN_EQUALS] = OTTER_PRECEDENCE_EQUALS,
    [OTTER_TOKEN_MINUS] = OTTER_PRECEDENCE_SUM,
    [OTTER_TOKEN_PLUS] = OTTER_PRECEDENCE_SUM,
    [OTTER_TOKEN_LEFT_PAREN] = OTTER_PRECEDENCE_PREFIX,
    [OTTER_TOKEN_INCREMENT] = OTTER_PRECEDENCE_SUM,
};

typedef otter_node *(*otter_parser_prefix_parse_fn)(
    otter_parser *parser, otter_parser_precedence precedence);
typedef otter_node *(*otter_parser_infix_parse_fn)(
    otter_parser *parser, otter_parser_precedence precedence,
    otter_node *left_expr);

/* Main drivers */
static otter_node *otter_parser_parse_statement(otter_parser *parser);
static otter_node *
otter_parser_parse_expression(otter_parser *parser,
                              otter_parser_precedence precedence);

/* Prefix parser functions */
static otter_node *
otter_parser_parse_integer(otter_parser *parser,
                           otter_parser_precedence precedence);
static otter_node *
otter_parser_parse_identifier(otter_parser *parser,
                              otter_parser_precedence precedence);
static otter_node *
otter_parser_parse_prefix_increment(otter_parser *parser,
                                    otter_parser_precedence precedence);

/* INfix parser functions */
static otter_node *
otter_parser_parse_addition(otter_parser *parser,
                            otter_parser_precedence precedence,
                            otter_node *left_expr);
static otter_node *
otter_parser_parse_postfix_increment(otter_parser *parser,
                                     otter_parser_precedence precedence,
                                     otter_node *left_expr);

static otter_parser_prefix_parse_fn
otter_find_prefix_parse_fn(otter_token *token) {
  switch (token->type) {
  case OTTER_TOKEN_IDENTIFIER:
    return otter_parser_parse_identifier;
  case OTTER_TOKEN_INTEGER:
    return otter_parser_parse_integer;
  case OTTER_TOKEN_INCREMENT:
    return otter_parser_parse_prefix_increment;
  default:
    return NULL;
  }
}

static otter_parser_infix_parse_fn
otter_find_infix_parse_fn(otter_token *token) {
  switch (token->type) {
  case OTTER_TOKEN_PLUS:
    return otter_parser_parse_addition;
  case OTTER_TOKEN_INCREMENT:
    return otter_parser_parse_postfix_increment;
  default:
    return NULL;
  }
}

#define NEXT_TOKEN_OR_RETURN_NULL(parser)                                      \
  ({                                                                           \
    if (parser->tokens_index >= parser->tokens_length) {                       \
      return NULL;                                                             \
    }                                                                          \
    otter_token *token_ = parser->tokens[parser->tokens_index];                \
    token_;                                                                    \
  })

#define NEXT_TOKEN_OR_GOTO_FAILURE(parser)                                     \
  ({                                                                           \
    if (parser->tokens_index >= parser->tokens_length) {                       \
      goto failure;                                                            \
    }                                                                          \
    otter_token *token_ = parser->tokens[parser->tokens_index];                \
    token_;                                                                    \
  })

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

static otter_node *otter_parser_parse_integer(otter_parser *parser,
                                              otter_parser_precedence) {
  if (parser == NULL) {
    return NULL;
  }

  if (parser->tokens_index >= parser->tokens_length) {
    return NULL;
  }

  otter_token *token = parser->tokens[parser->tokens_index];
  if (token->type != OTTER_TOKEN_INTEGER) {
    return NULL;
  }

  otter_token_integer *integer = (otter_token_integer *)token;
  otter_node_integer *integer_node =
      otter_malloc(parser->allocator, sizeof(*integer_node));
  if (integer_node == NULL) {
    return NULL;
  }

  integer_node->base.type = OTTER_NODE_INTEGER;
  integer_node->value = integer->value;
  parser->tokens_index++;
  return (otter_node *)integer_node;
}

static otter_node *otter_parser_parse_identifier(otter_parser *parser,
                                                 otter_parser_precedence) {
  if (parser == NULL) {
    return NULL;
  }

  otter_token *token = NEXT_TOKEN_OR_RETURN_NULL(parser);
  if (token->type != OTTER_TOKEN_IDENTIFIER) {
    return NULL;
  }

  otter_token_identifier *ident = (otter_token_identifier *)token;
  otter_node_identifier *ident_node =
      otter_malloc(parser->allocator, sizeof(*ident_node));
  if (ident_node == NULL) {
    return NULL;
  }

  ident_node->base.type = OTTER_NODE_IDENTIFIER;
  ident_node->value = otter_strdup(parser->allocator, ident->value);
  parser->tokens_index++;
  return (otter_node *)ident_node;
}

static otter_node *
otter_parser_parse_prefix_increment(otter_parser *parser,
                                    otter_parser_precedence precedence) {
  if (parser == NULL) {
    return NULL;
  }

  otter_token *token = NEXT_TOKEN_OR_RETURN_NULL(parser);
  if (token->type != OTTER_TOKEN_INCREMENT) {
    return NULL;
  }

  parser->tokens_index++;
  otter_node_increment *increment =
      otter_malloc(parser->allocator, sizeof(*increment));
  increment->base.type = OTTER_NODE_EXPRESSION_INCREMENT;
  if (increment == NULL) {
    return NULL;
  }

  increment->value = otter_parser_parse_expression(parser, precedence);
  if (increment->value == NULL) {
    otter_node_free(parser->allocator, (otter_node *)increment);
    return NULL;
  }

  return (otter_node *)increment;
}

static otter_node *otter_parser_parse_addition(otter_parser *parser,
                                               otter_parser_precedence,
                                               otter_node *left_expr) {
  otter_token *token = NEXT_TOKEN_OR_RETURN_NULL(parser);
  if (token->type != OTTER_TOKEN_PLUS) {
    return NULL;
  }

  parser->tokens_index++;
  otter_node_add *addition = otter_malloc(parser->allocator, sizeof(*addition));
  if (addition == NULL) {
    return NULL;
  }

  addition->base.type = OTTER_NODE_EXPRESSION_ADD;
  addition->left = left_expr;
  addition->right = otter_parser_parse_expression(parser, OTTER_PRECEDENCE_SUM);
  if (addition->right == NULL) {
    otter_free(parser->allocator, addition);
  }

  return (otter_node *)addition;
}

static otter_node *otter_parser_parse_postfix_increment(otter_parser *parser,
                                                        otter_parser_precedence,
                                                        otter_node *left_expr) {
  otter_token *token = NEXT_TOKEN_OR_RETURN_NULL(parser);
  if (token->type != OTTER_TOKEN_INCREMENT) {
    return NULL;
  }

  parser->tokens_index++;
  otter_node_increment *increment =
      otter_malloc(parser->allocator, sizeof(*increment));
  if (increment == NULL) {
    return NULL;
  }

  increment->base.type = OTTER_NODE_EXPRESSION_INCREMENT;
  increment->value = left_expr;
  return (otter_node *)increment;
}

static otter_node *
otter_parser_parse_expression(otter_parser *parser,
                              otter_parser_precedence precedence) {
  if (parser == NULL) {
    return NULL;
  }

  otter_token *token = NEXT_TOKEN_OR_RETURN_NULL(parser);
  otter_parser_prefix_parse_fn prefix_parse_fn =
      otter_find_prefix_parse_fn(token);
  if (prefix_parse_fn == NULL) {
    return NULL;
  }

  otter_node *left_expr = prefix_parse_fn(parser, precedence);
  token = NEXT_TOKEN_OR_GOTO_FAILURE(parser);
  while (token->type != OTTER_TOKEN_SEMICOLON &&
         precedence < otter_precedence_table[token->type]) {
    otter_parser_infix_parse_fn infix_parse_fn =
        otter_find_infix_parse_fn(token);
    if (infix_parse_fn == NULL) {
      return left_expr;
    }

    left_expr = infix_parse_fn(parser, precedence, left_expr);
    token = NEXT_TOKEN_OR_GOTO_FAILURE(parser);
  }

  return left_expr;

failure:
  otter_node_free(parser->allocator, left_expr);
  return NULL;
}

static otter_node_assignment *
otter_parser_parse_assignment_statement(otter_parser *parser) {
  otter_node_identifier *var_name = NULL;
  otter_node *expr = NULL;

  if (parser == NULL) {
    return NULL;
  }

  otter_token *token = NEXT_TOKEN_OR_RETURN_NULL(parser);
  if (token->type != OTTER_TOKEN_VAR) {
    return NULL;
  }

  parser->tokens_index++;
  var_name = (otter_node_identifier *)otter_parser_parse_identifier(
      parser, OTTER_PRECEDENCE_LOWEST);
  if (var_name == NULL) {
    return NULL;
  }

  token = NEXT_TOKEN_OR_GOTO_FAILURE(parser);
  if (token->type != OTTER_TOKEN_ASSIGNMENT) {
    goto failure;
  }

  parser->tokens_index++;
  expr = otter_parser_parse_expression(parser, OTTER_PRECEDENCE_LOWEST);
  if (expr == NULL) {
    goto failure;
  }

  token = NEXT_TOKEN_OR_GOTO_FAILURE(parser);
  if (token->type != OTTER_TOKEN_SEMICOLON) {
    goto failure;
  }

  parser->tokens_index++;
  otter_node_assignment *assignment_statement =
      otter_malloc(parser->allocator, sizeof(*assignment_statement));
  if (assignment_statement == NULL) {
    goto failure;
  }

  assignment_statement->base.type = OTTER_NODE_STATEMENT_ASSIGNMENT;
  assignment_statement->variable = var_name;
  assignment_statement->value_expr = expr;
  return assignment_statement;

failure:
  otter_node_free(parser->allocator, (otter_node *)var_name);
  otter_node_free(parser->allocator, expr);
  return NULL;
}

static otter_node_for *otter_parser_parse_for_statement(otter_parser *parser) {
  if (parser == NULL) {
    return NULL;
  }

  otter_node_for *for_loop = otter_malloc(parser->allocator, sizeof(*for_loop));
  if (for_loop == NULL) {
    return NULL;
  }

  for_loop->base.type = OTTER_NODE_STATEMENT_FOR;
  if (parser->tokens_index >= parser->tokens_length) {
    return NULL;
  }

  otter_token *token = NEXT_TOKEN_OR_RETURN_NULL(parser);
  parser->tokens_index++;
  if (token->type != OTTER_TOKEN_FOR) {
    goto failure;
  }

  for_loop->assignment = otter_parser_parse_assignment_statement(parser);
  if (for_loop->assignment == NULL) {
    goto failure;
  }

  for_loop->condition =
      otter_parser_parse_expression(parser, OTTER_PRECEDENCE_LOWEST);
  token = NEXT_TOKEN_OR_GOTO_FAILURE(parser);
  if (token->type != OTTER_TOKEN_SEMICOLON) {
    goto failure;
  }

  for_loop->iteration =
      otter_parser_parse_expression(parser, OTTER_PRECEDENCE_LOWEST);
  token = NEXT_TOKEN_OR_GOTO_FAILURE(parser);
  if (token->type != OTTER_TOKEN_SEMICOLON) {
    goto failure;
  }

  token = NEXT_TOKEN_OR_GOTO_FAILURE(parser);
  if (token->type != OTTER_TOKEN_LEFT_BRACKET) {
    goto failure;
  }

  while (true) {
    if (parser->tokens_index >= parser->tokens_length) {
      goto failure;
    }

    token = parser->tokens[parser->tokens_index];
    if (token->type == OTTER_TOKEN_RIGHT_BRACKET) {
      parser->tokens_index++;
      break;
    }

    otter_node *statement = otter_parser_parse_statement(parser);
    if (statement == NULL) {
      goto failure;
    }

    if (!OTTER_ARRAY_APPEND(for_loop, statements, parser->allocator,
                            statement)) {
      goto failure;
    }
  }

  return for_loop;
failure:
  otter_node_free(parser->allocator, (otter_node *)for_loop);
  return NULL;
}

static otter_node *otter_parser_parse_statement(otter_parser *parser) {
  if (parser == NULL) {
    return NULL;
  }

  otter_token *token = NEXT_TOKEN_OR_RETURN_NULL(parser);

  /* Only tokens that would start a statement are handled */
  switch (token->type) {
  case OTTER_TOKEN_VAR: {
    return (otter_node *)otter_parser_parse_assignment_statement(parser);
  } break;
  case OTTER_TOKEN_FOR: {
    return (otter_node *)otter_parser_parse_for_statement(parser);
  } break;
  case OTTER_TOKEN_IF: {
  } break;
  case OTTER_TOKEN_DEFINE_FUNCTION: {
  } break;
  case OTTER_TOKEN_CALL_FUNCTION: {
  } break;
  default: {
    otter_node *expr =
        otter_parser_parse_expression(parser, OTTER_PRECEDENCE_LOWEST);
    if (expr == NULL) {
      return NULL;
    }

    token = NEXT_TOKEN_OR_RETURN_NULL(parser);
    if (token->type != OTTER_TOKEN_SEMICOLON) {
      return NULL;
    }

    parser->tokens_index++;
    return expr;
  }
  }

  return NULL;
}

typedef struct otter_node_array {
  OTTER_ARRAY_DECLARE(otter_node *, nodes);
} otter_node_array;

otter_node **otter_parser_parse(otter_parser *parser, size_t *nodes_length) {
  if (parser == NULL || nodes_length == NULL) {
    return NULL;
  }

  otter_node_array result;
  OTTER_ARRAY_INIT(&result, nodes, parser->allocator);
  if (result.nodes == NULL) {
    return NULL;
  }

  while (parser->tokens_index < parser->tokens_length) {
    otter_node *statement = otter_parser_parse_statement(parser);
    if (statement == NULL) {
      goto failure;
    }

    if (!OTTER_ARRAY_APPEND(&result, nodes, parser->allocator, statement)) {
      goto failure;
    }
  }

  *nodes_length = result.nodes_length;
  return result.nodes;

failure:
  for (size_t i = 0; i < result.nodes_length; i++) {
    otter_node_free(parser->allocator, result.nodes[i]);
  }

  otter_free(parser->allocator, result.nodes);
  return NULL;
}
