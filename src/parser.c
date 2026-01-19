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
#include "otter/parser.h"
#include "otter/array.h"
#include "otter/cstring.h"
#include <assert.h>
#include <limits.h>

typedef enum otter_parser_precedence_level {
  OTTER_PARSER_PRECEDENCE_LEVEL_0,
  OTTER_PARSER_PRECEDENCE_LEVEL_1,
  OTTER_PARSER_PRECEDENCE_LEVEL_2,
  OTTER_PARSER_PRECEDENCE_LEVEL_3,
  OTTER_PARSER_PRECEDENCE_LEVEL_4,
  OTTER_PARSER_PRECEDENCE_LEVEL_5,
  OTTER_PARSER_PRECEDENCE_LEVEL_6,
  OTTER_PARSER_PRECEDENCE_LEVEL_7,
  OTTER_PARSER_PRECEDENCE_LEVEL_8,
  OTTER_PARSER_PRECEDENCE_LEVEL_9,
  OTTER_PARSER_PRECEDENCE_LEVEL_10,
} otter_parser_precedence_level;

typedef otter_node *(*otter_parser_prefix_parse_fn)(otter_parser *parser,
                                                    int min_precedence);
typedef otter_node *(*otter_parser_postfix_parse_fn)(otter_parser *parser,
                                                     otter_node *left_expr,
                                                     int min_precedence);
typedef otter_node *(*otter_parser_infix_parse_fn)(otter_parser *parser,
                                                   otter_node *left_expr,
                                                   int min_precedence);

/* Main drivers */
static otter_node *otter_parser_parse_statement(otter_parser *parser);
static otter_node *otter_parser_parse_expression(otter_parser *parser,
                                                 int min_precedence);

/* Prefix parser functions */
static otter_node *otter_parser_parse_integer(otter_parser *parser,
                                              int min_precedence);
static otter_node *otter_parser_parse_identifier(otter_parser *parser,
                                                 int min_precedence);
static otter_node *otter_parser_parse_prefix_increment(otter_parser *parser,
                                                       int min_precedence);
static otter_node *otter_parser_parse_prefix_decrement(otter_parser *parser,
                                                       int min_precedence);
static otter_node *otter_parser_parse_parens(otter_parser *parser,
                                             int min_precedence);

/* INfix parser functions */
static otter_node *otter_parser_parse_addition(otter_parser *parser,
                                               otter_node *left_expr,
                                               int min_precedence);
static otter_node *otter_parser_parse_subtract(otter_parser *parser,
                                               otter_node *left_expr,
                                               int min_precedence);
static otter_node *otter_parser_parse_multiply(otter_parser *parser,
                                               otter_node *left_expr,
                                               int min_precedence);
static otter_node *otter_parser_parse_divide(otter_parser *parser,
                                             otter_node *left_expr,
                                             int min_precedence);

/* Postfix parser functions */
static otter_node *otter_parser_parse_postfix_increment(otter_parser *parser,
                                                        otter_node *left_expr,
                                                        int min_precedence);
static otter_node *otter_parser_parse_postfix_decrement(otter_parser *parser,
                                                        otter_node *left_expr,
                                                        int min_precedence);

static bool otter_parser_get_prefix_precedence(otter_token *token,
                                               int *right_precedence) {
  if (right_precedence == NULL) {
    return false;
  }

  switch (token->type) {
  case OTTER_TOKEN_LEFT_PAREN:
    *right_precedence = OTTER_PARSER_PRECEDENCE_LEVEL_1;
    return true;
  case OTTER_TOKEN_IDENTIFIER:
  case OTTER_TOKEN_INTEGER:
    *right_precedence = OTTER_PARSER_PRECEDENCE_LEVEL_3;
    return true;
  case OTTER_TOKEN_INCREMENT:
  case OTTER_TOKEN_DECREMENT:
    *right_precedence = OTTER_PARSER_PRECEDENCE_LEVEL_5;
    return true;
  default:
    return false;
  }
}

static bool otter_parser_get_infix_precedence(otter_token *token,
                                              int *left_precedence,
                                              int *right_precedence) {
  if (left_precedence == NULL || right_precedence == NULL) {
    return false;
  }

  switch (token->type) {
  case OTTER_TOKEN_MINUS:
  case OTTER_TOKEN_PLUS:
    *left_precedence = 1;
    *right_precedence = 2;
    return true;
  case OTTER_TOKEN_MULTIPLY:
  case OTTER_TOKEN_DIVIDE:
    *left_precedence = 3;
    *right_precedence = 4;
    return true;
  default:
    return false;
  }
}

static bool otter_parser_get_postfix_precedence(otter_token *token,
                                                int *left_precedence) {
  if (left_precedence == NULL) {
    return false;
  }

  switch (token->type) {
  case OTTER_TOKEN_INCREMENT:
  case OTTER_TOKEN_DECREMENT:
    *left_precedence = OTTER_PARSER_PRECEDENCE_LEVEL_7;
    return true;

  case OTTER_TOKEN_RIGHT_PAREN:
    *left_precedence = 0;
    return true;
  default:
    return false;
  }
}

static otter_parser_prefix_parse_fn
otter_find_prefix_parse_fn(otter_token *token) {
  switch (token->type) {
  case OTTER_TOKEN_IDENTIFIER:
    return otter_parser_parse_identifier;
  case OTTER_TOKEN_INTEGER:
    return otter_parser_parse_integer;
  case OTTER_TOKEN_INCREMENT:
    return otter_parser_parse_prefix_increment;
  case OTTER_TOKEN_DECREMENT:
    return otter_parser_parse_prefix_decrement;
  case OTTER_TOKEN_LEFT_PAREN:
    return otter_parser_parse_parens;
  default:
    return NULL;
  }
}

static otter_parser_postfix_parse_fn
otter_find_postfix_parse_fn(otter_token *token) {
  switch (token->type) {
  case OTTER_TOKEN_INCREMENT:
    return otter_parser_parse_postfix_increment;
  case OTTER_TOKEN_DECREMENT:
    return otter_parser_parse_postfix_decrement;
  default:
    return NULL;
  }
}

static otter_parser_infix_parse_fn
otter_find_infix_parse_fn(otter_token *token) {
  switch (token->type) {
  case OTTER_TOKEN_PLUS:
    return otter_parser_parse_addition;
  case OTTER_TOKEN_MINUS:
    return otter_parser_parse_subtract;
  case OTTER_TOKEN_MULTIPLY:
    return otter_parser_parse_multiply;
  case OTTER_TOKEN_DIVIDE:
    return otter_parser_parse_divide;
  default:
    return NULL;
  }
}

#define NEXT_TOKEN_OR_RETURN_NULL(parser)                                      \
  ({                                                                           \
    if ((parser)->tokens_index >= (parser)->tokens_length) {                   \
      otter_log_error((parser)->logger,                                        \
                      "Parser's tokens_index of '%zd' exceeded the number of " \
                      "tokens provided, '%zd'",                                \
                      (parser)->tokens_index, (parser)->tokens_length);        \
      return NULL;                                                             \
    }                                                                          \
    otter_token *token_ = (parser)->tokens[(parser)->tokens_index];            \
    token_;                                                                    \
  })

#define NEXT_TOKEN_OR_GOTO_FAILURE(parser)                                     \
  ({                                                                           \
    if ((parser)->tokens_index >= (parser)->tokens_length) {                   \
      otter_log_error((parser)->logger,                                        \
                      "Parser's tokens_index of '%zd' exceeded the number of " \
                      "tokens provided, '%zd'",                                \
                      (parser)->tokens_index, (parser)->tokens_length);        \
      goto failure;                                                            \
    }                                                                          \
    otter_token *token_ = (parser)->tokens[(parser)->tokens_index];            \
    token_;                                                                    \
  })

otter_parser *otter_parser_create(otter_allocator *allocator,
                                  otter_token **tokens, size_t tokens_length,
                                  otter_logger *logger) {
  OTTER_RETURN_IF_NULL(logger, allocator, NULL);
  OTTER_RETURN_IF_NULL(logger, tokens, NULL);
  OTTER_RETURN_IF_NULL(logger, logger, NULL);

  otter_parser *parser = otter_malloc(allocator, sizeof(*parser));
  if (parser == NULL) {
    otter_log_critical(logger, "Unable to allocate '%zd' bytes for %s",
                       sizeof(*parser), OTTER_NAMEOF(parser));
    return NULL;
  }

  parser->allocator = allocator;
  parser->logger = logger;
  parser->tokens = tokens;
  parser->tokens_index = 0;
  parser->tokens_length = tokens_length;
  return parser;
}

void otter_parser_free(otter_parser *parser) {

  OTTER_ARRAY_FOREACH(parser, tokens, otter_token_free, parser->allocator);
  otter_free(parser->allocator, parser->tokens);
  otter_free(parser->allocator, parser);
}

OTTER_DEFINE_TRIVIAL_CLEANUP_FUNC(otter_parser *, otter_parser_free);

static otter_node *otter_parser_parse_integer(otter_parser *parser,
                                              int /* unused */) {
  if (parser == NULL) {
    return NULL;
  }

  otter_token *token = NEXT_TOKEN_OR_RETURN_NULL(parser);
  if (token->type != OTTER_TOKEN_INTEGER) {
    otter_log_error(
        parser->logger,
        "%d:%d: Expected next token to be an '%s', but encountered '%s'",
        token->line, token->column, otter_token_str(OTTER_TOKEN_INTEGER),
        otter_token_str(token->type));
    return NULL;
  }

  otter_token_integer *integer = (otter_token_integer *)token;
  otter_node_integer *integer_node =
      otter_malloc(parser->allocator, sizeof(*integer_node));
  if (integer_node == NULL) {
    otter_log_critical(parser->logger, "Unable to allocate '%zd' bytes",
                       sizeof(*integer_node));
    return NULL;
  }

  integer_node->base.type = OTTER_NODE_INTEGER;
  integer_node->value = integer->value;
  parser->tokens_index++;
  return (otter_node *)integer_node;
}

static otter_node *otter_parser_parse_identifier(otter_parser *parser,
                                                 int /* unused */) {
  if (parser == NULL) {
    return NULL;
  }

  otter_token *token = NEXT_TOKEN_OR_RETURN_NULL(parser);
  if (token->type != OTTER_TOKEN_IDENTIFIER) {
    otter_log_error(parser->logger,
                    "Expected next token to be an '%s', but encountered '%s'",
                    otter_token_str(OTTER_TOKEN_IDENTIFIER),
                    otter_token_str(token->type));
    return NULL;
  }

  otter_token_identifier *ident = (otter_token_identifier *)token;
  otter_node_identifier *ident_node =
      otter_malloc(parser->allocator, sizeof(*ident_node));
  if (ident_node == NULL) {
    otter_log_critical(parser->logger, "Unable to allocate '%zd' bytes for %s",
                       sizeof(*ident_node), OTTER_NAMEOF(ident_node));
    return NULL;
  }

  ident_node->base.type = OTTER_NODE_IDENTIFIER;
  ident_node->value = otter_strdup(parser->allocator, ident->value);
  parser->tokens_index++;
  return (otter_node *)ident_node;
}

static otter_node *otter_parser_parse_prefix_increment(otter_parser *parser,
                                                       int min_precedence) {
  if (parser == NULL) {
    return NULL;
  }

  otter_token *token = NEXT_TOKEN_OR_RETURN_NULL(parser);
  if (token->type != OTTER_TOKEN_INCREMENT) {
    otter_log_error(
        parser->logger,
        "%d:%d: Expected next token to be an '%s', but encountered '%s'",
        token->line, token->column, otter_token_str(OTTER_TOKEN_INCREMENT),
        otter_token_str(token->type));
    return NULL;
  }

  parser->tokens_index++;
  otter_node_unary_expr *increment =
      otter_malloc(parser->allocator, sizeof(*increment));
  if (increment == NULL) {
    otter_log_critical(parser->logger, "Unable to allocate '%zd' bytes",
                       sizeof(*increment));
    return NULL;
  }

  increment->base.type = OTTER_NODE_EXPRESSION_INCREMENT;
  increment->value = otter_parser_parse_expression(parser, min_precedence);
  if (increment->value == NULL) {
    otter_log_error(parser->logger,
                    "Unable to parse expression that was to be incremented");
    otter_node_free(parser->allocator, (otter_node *)increment);
    return NULL;
  }

  return (otter_node *)increment;
}

static otter_node *otter_parser_parse_prefix_decrement(otter_parser *parser,
                                                       int min_precedence) {
  if (parser == NULL) {
    return NULL;
  }

  otter_token *token = NEXT_TOKEN_OR_RETURN_NULL(parser);
  if (token->type != OTTER_TOKEN_DECREMENT) {
    otter_log_error(
        parser->logger,
        "%d:%d: Expected next token to be an '%s', but encountered '%s'",
        token->line, token->column, otter_token_str(OTTER_TOKEN_DECREMENT),
        otter_token_str(token->type));
    return NULL;
  }

  parser->tokens_index++;
  otter_node_unary_expr *decrement =
      otter_malloc(parser->allocator, sizeof(*decrement));
  if (decrement == NULL) {
    otter_log_critical(parser->logger, "Unable to allocate '%zd' bytes",
                       sizeof(*decrement));
    return NULL;
  }

  decrement->base.type = OTTER_NODE_EXPRESSION_DECREMENT;
  decrement->value = otter_parser_parse_expression(parser, min_precedence);
  if (decrement->value == NULL) {
    otter_log_error(parser->logger,
                    "Unable to parse expression that was to be decremented");
    otter_node_free(parser->allocator, (otter_node *)decrement);
    return NULL;
  }

  return (otter_node *)decrement;
}

static otter_node *otter_parser_parse_parens(otter_parser *parser,
                                             int min_precedence) {
  if (parser == NULL) {
    return NULL;
  }

  otter_token *token = NEXT_TOKEN_OR_RETURN_NULL(parser);
  if (token->type != OTTER_TOKEN_LEFT_PAREN) {
    otter_log_error(
        parser->logger,
        "%d:%d: Expected next token to be an '%s', but encountered '%s'",
        token->line, token->column, otter_token_str(OTTER_TOKEN_LEFT_PAREN),
        otter_token_str(token->type));
    return NULL;
  }

  parser->tokens_index++;
  otter_node *expr = otter_parser_parse_expression(parser, min_precedence);
  if (expr == NULL) {
    otter_log_error(parser->logger,
                    "Failed to parse expression between parenthesis");
    return NULL;
  }

  token = NEXT_TOKEN_OR_GOTO_FAILURE(parser);
  if (token->type != OTTER_TOKEN_RIGHT_PAREN) {
    otter_log_error(
        parser->logger,
        "%d:%d: Expected next token to be an '%s', but encountered '%s'",
        token->line, token->column, otter_token_str(OTTER_TOKEN_RIGHT_PAREN),
        otter_token_str(token->type));
    goto failure;
  }

  parser->tokens_index++;
  return expr;
failure:
  otter_node_free(parser->allocator, expr);
  return NULL;
}

static otter_node *otter_parser_parse_addition(otter_parser *parser,
                                               otter_node *left_expr,
                                               int min_precedence) {
  if (parser == NULL) {
    return NULL;
  }

  OTTER_RETURN_IF_NULL(parser->logger, left_expr, NULL);
  otter_token *token = NEXT_TOKEN_OR_RETURN_NULL(parser);
  if (token->type != OTTER_TOKEN_PLUS) {
    otter_log_error(
        parser->logger,
        "%d:%d: Expected next token to be an '%s', but encountered '%s'",
        token->line, token->column, otter_token_str(OTTER_TOKEN_PLUS),
        otter_token_str(token->type));
    return NULL;
  }

  parser->tokens_index++;
  otter_node_binary_expr *addition =
      otter_malloc(parser->allocator, sizeof(*addition));
  if (addition == NULL) {
    otter_log_critical(parser->logger, "Unable to allocate '%zd' bytes",
                       sizeof(*addition));
    return NULL;
  }

  addition->base.type = OTTER_NODE_EXPRESSION_ADD;
  addition->left = left_expr;
  addition->right = otter_parser_parse_expression(parser, min_precedence);
  if (addition->right == NULL) {
    otter_log_error(parser->logger,
                    "Unable to parse right expression within addition");
    otter_free(parser->allocator, addition);
    return NULL;
  }

  return (otter_node *)addition;
}

static otter_node *otter_parser_parse_subtract(otter_parser *parser,
                                               otter_node *left_expr,
                                               int min_precedence) {
  if (parser == NULL) {
    return NULL;
  }

  OTTER_RETURN_IF_NULL(parser->logger, left_expr, NULL);
  otter_token *token = NEXT_TOKEN_OR_RETURN_NULL(parser);
  if (token->type != OTTER_TOKEN_MINUS) {
    otter_log_error(
        parser->logger,
        "%d:%d: Expected next token to be an '%s', but encountered '%s'",
        token->line, token->column, otter_token_str(OTTER_TOKEN_MINUS),
        otter_token_str(token->type));
    return NULL;
  }

  parser->tokens_index++;
  otter_node_binary_expr *subtraction =
      otter_malloc(parser->allocator, sizeof(*subtraction));
  if (subtraction == NULL) {
    otter_log_critical(parser->logger, "Unable to allocate '%zd' bytes",
                       sizeof(*subtraction));
    return NULL;
  }

  subtraction->base.type = OTTER_NODE_EXPRESSION_SUBTRACT;
  subtraction->left = left_expr;
  subtraction->right = otter_parser_parse_expression(parser, min_precedence);
  if (subtraction->right == NULL) {
    otter_log_error(parser->logger,
                    "Unable to parse right expression within subtraction");
    otter_free(parser->allocator, subtraction);
    return NULL;
  }

  return (otter_node *)subtraction;
}

static otter_node *otter_parser_parse_multiply(otter_parser *parser,
                                               otter_node *left_expr,
                                               int min_precedence) {
  if (parser == NULL) {
    return NULL;
  }
  OTTER_RETURN_IF_NULL(parser->logger, left_expr, NULL);
  otter_token *token = NEXT_TOKEN_OR_RETURN_NULL(parser);
  if (token->type != OTTER_TOKEN_MULTIPLY) {
    otter_log_error(
        parser->logger,
        "%d:%d: Expected next token to be an '%s', but encountered '%s'",
        token->line, token->column, otter_token_str(OTTER_TOKEN_MULTIPLY),
        otter_token_str(token->type));
    return NULL;
  }

  parser->tokens_index++;
  otter_node_binary_expr *multiply =
      otter_malloc(parser->allocator, sizeof(*multiply));
  if (multiply == NULL) {
    otter_log_error(parser->logger, "Unable to allocate '%zd' bytes",
                    sizeof(*multiply));
    return NULL;
  }

  multiply->base.type = OTTER_NODE_EXPRESSION_MULTIPLY;
  multiply->left = left_expr;
  multiply->right = otter_parser_parse_expression(parser, min_precedence);
  if (multiply->right == NULL) {
    otter_log_error(parser->logger,
                    "Unable to parse right expression within multiply");
    otter_free(parser->allocator, multiply);
    return NULL;
  }

  return (otter_node *)multiply;
}

static otter_node *otter_parser_parse_divide(otter_parser *parser,
                                             otter_node *left_expr,
                                             int min_precedence) {
  if (parser == NULL) {
    return NULL;
  }

  OTTER_RETURN_IF_NULL(parser->logger, left_expr, NULL);
  otter_token *token = NEXT_TOKEN_OR_RETURN_NULL(parser);
  if (token->type != OTTER_TOKEN_DIVIDE) {
    otter_log_error(
        parser->logger,
        "%d:%d: Expected next token to be an '%s', but encountered '%s'",
        token->line, token->column, otter_token_str(OTTER_TOKEN_DIVIDE),
        otter_token_str(token->type));
    return NULL;
  }

  parser->tokens_index++;
  otter_node_binary_expr *divide =
      otter_malloc(parser->allocator, sizeof(*divide));
  if (divide == NULL) {
    otter_log_error(parser->logger, "Unable to allocate '%zd' bytes",
                    sizeof(*divide));
    return NULL;
  }

  divide->base.type = OTTER_NODE_EXPRESSION_DIVIDE;
  divide->left = left_expr;
  divide->right = otter_parser_parse_expression(parser, min_precedence);
  if (divide->right == NULL) {
    otter_log_error(parser->logger,
                    "Unable to parse right expression within divide");
    otter_free(parser->allocator, divide);
    return NULL;
  }

  return (otter_node *)divide;
}

static otter_node *otter_parser_parse_postfix_increment(otter_parser *parser,
                                                        otter_node *left_expr,
                                                        int /* unused */) {
  if (parser == NULL) {
    return NULL;
  }

  OTTER_RETURN_IF_NULL(parser->logger, left_expr, NULL);
  otter_token *token = NEXT_TOKEN_OR_RETURN_NULL(parser);
  if (token->type != OTTER_TOKEN_INCREMENT) {
    otter_log_error(
        parser->logger,
        "%d:%d: Expected next token to be an '%s', but encountered '%s'",
        token->line, token->column, otter_token_str(OTTER_TOKEN_INCREMENT),
        otter_token_str(token->type));
    return NULL;
  }

  parser->tokens_index++;
  otter_node_unary_expr *increment =
      otter_malloc(parser->allocator, sizeof(*increment));
  if (increment == NULL) {
    otter_log_error(parser->logger, "Unable to allocate '%zd' bytes",
                    sizeof(*increment));
    return NULL;
  }

  increment->base.type = OTTER_NODE_EXPRESSION_INCREMENT;
  increment->value = left_expr;
  return (otter_node *)increment;
}

static otter_node *otter_parser_parse_postfix_decrement(otter_parser *parser,
                                                        otter_node *left_expr,
                                                        int /* unused */) {
  if (parser == NULL) {
    return NULL;
  }

  OTTER_RETURN_IF_NULL(parser->logger, left_expr, NULL);
  otter_token *token = NEXT_TOKEN_OR_RETURN_NULL(parser);
  if (token->type != OTTER_TOKEN_DECREMENT) {
    otter_log_error(
        parser->logger,
        "%d:%d: Expected next token to be an '%s', but encountered '%s'",
        token->line, token->column, otter_token_str(OTTER_TOKEN_DECREMENT),
        otter_token_str(token->type));
    return NULL;
  }

  parser->tokens_index++;
  otter_node_unary_expr *decrement =
      otter_malloc(parser->allocator, sizeof(*decrement));
  if (decrement == NULL) {
    otter_log_error(parser->logger, "Unable to allocate '%zd' bytes",
                    sizeof(*decrement));
    return NULL;
  }

  decrement->base.type = OTTER_NODE_EXPRESSION_DECREMENT;
  decrement->value = left_expr;
  return (otter_node *)decrement;
}

static otter_node *otter_parser_parse_expression(otter_parser *parser,
                                                 int min_precedence) {
  if (parser == NULL) {
    return NULL;
  }

  otter_token *token = NEXT_TOKEN_OR_RETURN_NULL(parser);
  otter_parser_prefix_parse_fn prefix_parse_fn =
      otter_find_prefix_parse_fn(token);
  if (prefix_parse_fn == NULL) {
    otter_log_error(parser->logger,
                    "%d:%d: Function does not exist to parse token '%s'",
                    token->line, token->column, otter_token_str(token->type));
    return NULL;
  }

  int prefix_right_precedence = 0;
  if (!otter_parser_get_prefix_precedence(token, &prefix_right_precedence)) {
    otter_log_error(parser->logger,
                    "Unable to find precedence ordering for token '%s'",
                    otter_token_str(token->type));
    return NULL;
  }

  otter_node *left_expr = prefix_parse_fn(parser, prefix_right_precedence);
  if (left_expr == NULL) {
    return NULL;
  }

  token = NEXT_TOKEN_OR_GOTO_FAILURE(parser);
  while (token->type != OTTER_TOKEN_SEMICOLON) {
    int postfix_left_precedence = 0;
    if (otter_parser_get_postfix_precedence(token, &postfix_left_precedence)) {
      if (postfix_left_precedence < min_precedence) {
        otter_log_debug(parser->logger,
                        "'%s' of %d is less than '%s' of %d, finishing parsing",
                        OTTER_NAMEOF(postfix_left_precedence),
                        postfix_left_precedence, OTTER_NAMEOF(min_precedence),
                        min_precedence);
        break;
      }

      otter_parser_postfix_parse_fn postfix_parse_fn =
          otter_find_postfix_parse_fn(token);
      if (postfix_parse_fn == NULL) {
        otter_log_error(parser->logger,
                        "Unable to find parsing function for token '%s'",
                        token->type);
        return left_expr;
      }

      left_expr = postfix_parse_fn(parser, left_expr, postfix_left_precedence);
      if (left_expr == NULL) {
        return NULL;
      }
    } else {
      int infix_left_precedence = 0;
      int infix_right_precedence = 0;
      if (!otter_parser_get_infix_precedence(token, &infix_left_precedence,
                                             &infix_right_precedence)) {
        otter_log_error(parser->logger,
                        "Unable to find precedence ordering for token '%s'",
                        otter_token_str(token->type));
        goto failure;
      }

      if (infix_left_precedence < min_precedence) {
        otter_log_debug(parser->logger,
                        "'%s' of %d is less than '%s' of %d, finishing parsing",
                        OTTER_NAMEOF(infix_left_precedence),
                        infix_left_precedence, OTTER_NAMEOF(min_precedence),
                        min_precedence);
        break;
      }

      otter_parser_infix_parse_fn infix_parse_fn =
          otter_find_infix_parse_fn(token);
      if (infix_parse_fn == NULL) {
        otter_log_error(parser->logger,
                        "Unable to find parsing function for token '%s'",
                        token->type);
        return left_expr;
      }

      left_expr = infix_parse_fn(parser, left_expr, infix_right_precedence);
      if (left_expr == NULL) {
        return NULL;
      }
    }
    token = NEXT_TOKEN_OR_GOTO_FAILURE(parser);
  }

  return left_expr;

failure:
  otter_node_free(parser->allocator, left_expr);
  return NULL;
}

static otter_node_assignment *
otter_parser_parse_assignment_statement(otter_parser *parser) {
  if (parser == NULL) {
    return NULL;
  }

  otter_node_identifier *var_name = NULL;
  otter_node *expr = NULL;

  otter_token *token = NEXT_TOKEN_OR_RETURN_NULL(parser);
  if (token->type != OTTER_TOKEN_VAR) {
    return NULL;
  }

  parser->tokens_index++;
  var_name = (otter_node_identifier *)otter_parser_parse_identifier(parser, 0);
  if (var_name == NULL) {
    return NULL;
  }

  token = NEXT_TOKEN_OR_GOTO_FAILURE(parser);
  if (token->type != OTTER_TOKEN_ASSIGNMENT) {
    goto failure;
  }

  parser->tokens_index++;
  expr = otter_parser_parse_expression(parser, 0);
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
    otter_log_error(parser->logger, "Unable to allocate %zd bytes of %s",
                    sizeof(*for_loop), OTTER_NAMEOF(for_loop));
    return NULL;
  }

  for_loop->base.type = OTTER_NODE_STATEMENT_FOR;
  otter_token *token = NEXT_TOKEN_OR_RETURN_NULL(parser);
  parser->tokens_index++;
  if (token->type != OTTER_TOKEN_FOR) {
    goto failure;
  }

  for_loop->assignment = otter_parser_parse_assignment_statement(parser);
  if (for_loop->assignment == NULL) {
    goto failure;
  }

  for_loop->condition = otter_parser_parse_expression(parser, 0);
  token = NEXT_TOKEN_OR_GOTO_FAILURE(parser);
  if (token->type != OTTER_TOKEN_SEMICOLON) {
    otter_log_error(
        parser->logger,
        "%d:%d: Expected next token to be an '%s', but encountered '%s'",
        token->line, token->column, otter_token_str(OTTER_TOKEN_SEMICOLON),
        otter_token_str(token->type));
    goto failure;
  }

  for_loop->iteration = otter_parser_parse_expression(parser, 0);
  token = NEXT_TOKEN_OR_GOTO_FAILURE(parser);
  if (token->type != OTTER_TOKEN_SEMICOLON) {
    otter_log_error(
        parser->logger,
        "%d:%d: Expected next token to be an '%s', but encountered '%s'",
        token->line, token->column, otter_token_str(OTTER_TOKEN_SEMICOLON),
        otter_token_str(token->type));
    goto failure;
  }

  token = NEXT_TOKEN_OR_GOTO_FAILURE(parser);
  if (token->type != OTTER_TOKEN_LEFT_BRACKET) {
    otter_log_error(
        parser->logger,
        "%d:%d: Expected next token to be an '%s', but encountered '%s'",
        token->line, token->column, otter_token_str(OTTER_TOKEN_LEFT_BRACKET),
        otter_token_str(token->type));
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
  case OTTER_TOKEN_IF:
  case OTTER_TOKEN_DEFINE_FUNCTION:
  case OTTER_TOKEN_CALL_FUNCTION:
    /* Not yet implemented */
    break;
  default: {
    otter_node *expr = otter_parser_parse_expression(parser, 0);
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
  if (parser == NULL) {
    return NULL;
  }

  OTTER_RETURN_IF_NULL(parser->logger, nodes_length, NULL);
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
  OTTER_ARRAY_FOREACH(&result, nodes, otter_node_free, parser->allocator);
  otter_free(parser->allocator, result.nodes);
  return NULL;
}
