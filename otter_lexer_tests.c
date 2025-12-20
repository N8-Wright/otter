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
#include "otter_test.h"
#include "otter_token.h"
#include <string.h>

OTTER_TEST(lexer_create_null_allocator) {
  otter_lexer *lexer = otter_lexer_create(NULL, "");
  OTTER_ASSERT(lexer == NULL);
  OTTER_TEST_END();
}

OTTER_TEST(lexer_create_null_source) {
  otter_lexer *lexer = otter_lexer_create(OTTER_TEST_ALLOCATOR, NULL);
  OTTER_ASSERT(lexer == NULL);
  OTTER_TEST_END();
}

static void *null_malloc(otter_allocator *, size_t) { return NULL; }

OTTER_TEST(lexer_create_allocator_returns_null) {
  otter_allocator_vtable vtable = {
      .malloc = null_malloc,
      .realloc = NULL,
      .free = NULL,
  };

  otter_allocator allocator = {
      .vtable = &vtable,
  };

  otter_lexer *lexer = otter_lexer_create(&allocator, "source");
  OTTER_ASSERT(lexer == NULL);
  OTTER_TEST_END();
}

OTTER_TEST(lexer_tokenize_null_lexer) {
  size_t tokens_length;
  otter_token **tokens = otter_lexer_tokenize(NULL, &tokens_length);

  OTTER_ASSERT(tokens == NULL);

  OTTER_TEST_END();
}

OTTER_TEST(lexer_tokenize_null_size) {
  OTTER_CLEANUP(otter_lexer_free_p)
  otter_lexer *lexer = otter_lexer_create(OTTER_TEST_ALLOCATOR, "");
  otter_token **tokens = otter_lexer_tokenize(lexer, NULL);

  OTTER_ASSERT(tokens == NULL);
  OTTER_TEST_END();
}

OTTER_TEST(lexer_tokenize_left_paren) {
  OTTER_CLEANUP(otter_lexer_free_p)
  otter_lexer *lexer = otter_lexer_create(OTTER_TEST_ALLOCATOR, "(");
  size_t tokens_length = 0;
  otter_token **tokens = otter_lexer_tokenize(lexer, &tokens_length);

  OTTER_ASSERT(tokens_length == 1);
  OTTER_ASSERT(tokens[0]->type == OTTER_TOKEN_LEFT_PAREN);

  OTTER_TEST_END(for (size_t i = 0; i < tokens_length; i++) {
    otter_token_free(OTTER_TEST_ALLOCATOR, tokens[i]);
  } otter_free(OTTER_TEST_ALLOCATOR, tokens););
}

OTTER_TEST(lexer_tokenize_right_paren) {
  OTTER_CLEANUP(otter_lexer_free_p)
  otter_lexer *lexer = otter_lexer_create(OTTER_TEST_ALLOCATOR, ")");
  size_t tokens_length = 0;
  otter_token **tokens = otter_lexer_tokenize(lexer, &tokens_length);

  OTTER_ASSERT(tokens_length == 1);
  OTTER_ASSERT(tokens[0]->type == OTTER_TOKEN_RIGHT_PAREN);

  OTTER_TEST_END(for (size_t i = 0; i < tokens_length; i++) {
    otter_token_free(OTTER_TEST_ALLOCATOR, tokens[i]);
  } otter_free(OTTER_TEST_ALLOCATOR, tokens););
}

OTTER_TEST(lexer_tokenize_left_bracket) {
  OTTER_CLEANUP(otter_lexer_free_p)
  otter_lexer *lexer = otter_lexer_create(OTTER_TEST_ALLOCATOR, "{");
  size_t tokens_length = 0;
  otter_token **tokens = otter_lexer_tokenize(lexer, &tokens_length);

  OTTER_ASSERT(tokens_length == 1);
  OTTER_ASSERT(tokens[0]->type == OTTER_TOKEN_LEFT_BRACKET);

  OTTER_TEST_END(for (size_t i = 0; i < tokens_length; i++) {
    otter_token_free(OTTER_TEST_ALLOCATOR, tokens[i]);
  } otter_free(OTTER_TEST_ALLOCATOR, tokens););
}

OTTER_TEST(lexer_tokenize_right_bracket) {
  OTTER_CLEANUP(otter_lexer_free_p)
  otter_lexer *lexer = otter_lexer_create(OTTER_TEST_ALLOCATOR, "}");
  size_t tokens_length = 0;
  otter_token **tokens = otter_lexer_tokenize(lexer, &tokens_length);

  OTTER_ASSERT(tokens_length == 1);
  OTTER_ASSERT(tokens[0]->type == OTTER_TOKEN_RIGHT_BRACKET);

  OTTER_TEST_END(for (size_t i = 0; i < tokens_length; i++) {
    otter_token_free(OTTER_TEST_ALLOCATOR, tokens[i]);
  } otter_free(OTTER_TEST_ALLOCATOR, tokens););
}

OTTER_TEST(lexer_tokenize_equals) {
  OTTER_CLEANUP(otter_lexer_free_p)
  otter_lexer *lexer = otter_lexer_create(OTTER_TEST_ALLOCATOR, "==");
  size_t tokens_length = 0;
  otter_token **tokens = otter_lexer_tokenize(lexer, &tokens_length);

  OTTER_ASSERT(tokens_length == 1);
  OTTER_ASSERT(tokens[0]->type == OTTER_TOKEN_EQUALS);

  OTTER_TEST_END(for (size_t i = 0; i < tokens_length; i++) {
    otter_token_free(OTTER_TEST_ALLOCATOR, tokens[i]);
  } otter_free(OTTER_TEST_ALLOCATOR, tokens););
}

OTTER_TEST(lexer_tokenize_assignment) {
  OTTER_CLEANUP(otter_lexer_free_p)
  otter_lexer *lexer = otter_lexer_create(OTTER_TEST_ALLOCATOR, "=");
  size_t tokens_length = 0;
  otter_token **tokens = otter_lexer_tokenize(lexer, &tokens_length);

  OTTER_ASSERT(tokens_length == 1);
  OTTER_ASSERT(tokens[0]->type == OTTER_TOKEN_ASSIGNMENT);

  OTTER_TEST_END(for (size_t i = 0; i < tokens_length; i++) {
    otter_token_free(OTTER_TEST_ALLOCATOR, tokens[i]);
  } otter_free(OTTER_TEST_ALLOCATOR, tokens););
}

OTTER_TEST(lexer_tokenize_equals_then_assignment) {
  OTTER_CLEANUP(otter_lexer_free_p)
  otter_lexer *lexer = otter_lexer_create(OTTER_TEST_ALLOCATOR, "===");
  size_t tokens_length = 0;
  otter_token **tokens = otter_lexer_tokenize(lexer, &tokens_length);

  OTTER_ASSERT(tokens_length == 2);
  OTTER_ASSERT(tokens[0]->type == OTTER_TOKEN_EQUALS);
  OTTER_ASSERT(tokens[1]->type == OTTER_TOKEN_ASSIGNMENT);

  OTTER_TEST_END(for (size_t i = 0; i < tokens_length; i++) {
    otter_token_free(OTTER_TEST_ALLOCATOR, tokens[i]);
  } otter_free(OTTER_TEST_ALLOCATOR, tokens););
}

OTTER_TEST(lexer_tokenize_positive_integer) {
  OTTER_CLEANUP(otter_lexer_free_p)
  otter_lexer *lexer = otter_lexer_create(OTTER_TEST_ALLOCATOR, "1234");
  size_t tokens_length = 0;
  otter_token **tokens = otter_lexer_tokenize(lexer, &tokens_length);

  OTTER_ASSERT(tokens_length == 1);
  OTTER_ASSERT(tokens[0]->type == OTTER_TOKEN_INTEGER);
  OTTER_ASSERT(((otter_token_integer *)tokens[0])->value == 1234);

  OTTER_TEST_END(for (size_t i = 0; i < tokens_length; i++) {
    otter_token_free(OTTER_TEST_ALLOCATOR, tokens[i]);
  } otter_free(OTTER_TEST_ALLOCATOR, tokens););
}

OTTER_TEST(lexer_tokenize_negative_integer) {
  OTTER_CLEANUP(otter_lexer_free_p)
  otter_lexer *lexer = otter_lexer_create(OTTER_TEST_ALLOCATOR, "-1234");
  size_t tokens_length = 0;
  otter_token **tokens = otter_lexer_tokenize(lexer, &tokens_length);

  OTTER_ASSERT(tokens_length == 1);
  OTTER_ASSERT(tokens[0]->type == OTTER_TOKEN_INTEGER);
  OTTER_ASSERT(((otter_token_integer *)tokens[0])->value == -1234);

  OTTER_TEST_END(for (size_t i = 0; i < tokens_length; i++) {
    otter_token_free(OTTER_TEST_ALLOCATOR, tokens[i]);
  } otter_free(OTTER_TEST_ALLOCATOR, tokens););
}

OTTER_TEST(lexer_tokenize_minus_then_integer) {
  OTTER_CLEANUP(otter_lexer_free_p)
  otter_lexer *lexer = otter_lexer_create(OTTER_TEST_ALLOCATOR, "- 8910");
  size_t tokens_length = 0;
  otter_token **tokens = otter_lexer_tokenize(lexer, &tokens_length);

  OTTER_ASSERT(tokens_length == 2);
  OTTER_ASSERT(tokens[0]->type == OTTER_TOKEN_MINUS);
  OTTER_ASSERT(tokens[1]->type == OTTER_TOKEN_INTEGER);
  OTTER_ASSERT(((otter_token_integer *)tokens[1])->value == 8910);

  OTTER_TEST_END(for (size_t i = 0; i < tokens_length; i++) {
    otter_token_free(OTTER_TEST_ALLOCATOR, tokens[i]);
  } otter_free(OTTER_TEST_ALLOCATOR, tokens););
}

OTTER_TEST(lexer_tokenize_minus_then_negative_integer) {
  OTTER_CLEANUP(otter_lexer_free_p)
  otter_lexer *lexer = otter_lexer_create(OTTER_TEST_ALLOCATOR, "- -8911");
  size_t tokens_length = 0;
  otter_token **tokens = otter_lexer_tokenize(lexer, &tokens_length);

  OTTER_ASSERT(tokens_length == 2);
  OTTER_ASSERT(tokens[0]->type == OTTER_TOKEN_MINUS);
  OTTER_ASSERT(tokens[1]->type == OTTER_TOKEN_INTEGER);
  OTTER_ASSERT(((otter_token_integer *)tokens[1])->value == -8911);

  OTTER_TEST_END(for (size_t i = 0; i < tokens_length; i++) {
    otter_token_free(OTTER_TEST_ALLOCATOR, tokens[i]);
  } otter_free(OTTER_TEST_ALLOCATOR, tokens););
}

OTTER_TEST(lexer_tokenize_var) {
  OTTER_CLEANUP(otter_lexer_free_p)
  otter_lexer *lexer = otter_lexer_create(OTTER_TEST_ALLOCATOR, "var");
  size_t tokens_length = 0;
  otter_token **tokens = otter_lexer_tokenize(lexer, &tokens_length);

  OTTER_ASSERT(tokens_length == 1);
  OTTER_ASSERT(tokens[0]->type == OTTER_TOKEN_VAR);

  OTTER_TEST_END(for (size_t i = 0; i < tokens_length; i++) {
    otter_token_free(OTTER_TEST_ALLOCATOR, tokens[i]);
  } otter_free(OTTER_TEST_ALLOCATOR, tokens););
}

OTTER_TEST(lexer_tokenize_for) {
  OTTER_CLEANUP(otter_lexer_free_p)
  otter_lexer *lexer = otter_lexer_create(OTTER_TEST_ALLOCATOR, "for");
  size_t tokens_length = 0;
  otter_token **tokens = otter_lexer_tokenize(lexer, &tokens_length);

  OTTER_ASSERT(tokens_length == 1);
  OTTER_ASSERT(tokens[0]->type == OTTER_TOKEN_FOR);

  OTTER_TEST_END(for (size_t i = 0; i < tokens_length; i++) {
    otter_token_free(OTTER_TEST_ALLOCATOR, tokens[i]);
  } otter_free(OTTER_TEST_ALLOCATOR, tokens););
}

OTTER_TEST(lexer_tokenize_if) {
  OTTER_CLEANUP(otter_lexer_free_p)
  otter_lexer *lexer = otter_lexer_create(OTTER_TEST_ALLOCATOR, "if");
  size_t tokens_length = 0;
  otter_token **tokens = otter_lexer_tokenize(lexer, &tokens_length);

  OTTER_ASSERT(tokens_length == 1);
  OTTER_ASSERT(tokens[0]->type == OTTER_TOKEN_IF);

  OTTER_TEST_END(for (size_t i = 0; i < tokens_length; i++) {
    otter_token_free(OTTER_TEST_ALLOCATOR, tokens[i]);
  } otter_free(OTTER_TEST_ALLOCATOR, tokens););
}

OTTER_TEST(lexer_tokenize_else) {
  OTTER_CLEANUP(otter_lexer_free_p)
  otter_lexer *lexer = otter_lexer_create(OTTER_TEST_ALLOCATOR, "else");
  size_t tokens_length = 0;
  otter_token **tokens = otter_lexer_tokenize(lexer, &tokens_length);

  OTTER_ASSERT(tokens_length == 1);
  OTTER_ASSERT(tokens[0]->type == OTTER_TOKEN_ELSE);

  OTTER_TEST_END(for (size_t i = 0; i < tokens_length; i++) {
    otter_token_free(OTTER_TEST_ALLOCATOR, tokens[i]);
  } otter_free(OTTER_TEST_ALLOCATOR, tokens););
}

OTTER_TEST(lexer_tokenize_identifier) {
  OTTER_CLEANUP(otter_lexer_free_p)
  otter_lexer *lexer =
      otter_lexer_create(OTTER_TEST_ALLOCATOR, "some_id-entifier");
  size_t tokens_length = 0;
  otter_token **tokens = otter_lexer_tokenize(lexer, &tokens_length);

  OTTER_ASSERT(tokens_length == 1);
  OTTER_ASSERT(tokens[0]->type == OTTER_TOKEN_IDENTIFIER);
  OTTER_ASSERT(0 == strcmp(((otter_token_identifier *)tokens[0])->value,
                           "some_id-entifier"));

  OTTER_TEST_END(for (size_t i = 0; i < tokens_length; i++) {
    otter_token_free(OTTER_TEST_ALLOCATOR, tokens[i]);
  } otter_free(OTTER_TEST_ALLOCATOR, tokens););
}

typedef struct otter_allocator_mock_malloc_failure {
  otter_allocator base;
  otter_allocator *parent;
  otter_allocator_vtable mock_vtable;
  int num_malloc_calls;
  int start_failing_after;
} otter_allocator_mock_malloc_failure;

void *
otter_allocator_mock_malloc_failure_malloc_impl(otter_allocator *allocator,
                                                size_t size) {
  otter_allocator_mock_malloc_failure *mock =
      (otter_allocator_mock_malloc_failure *)allocator;
  mock->num_malloc_calls++;
  if (mock->num_malloc_calls > mock->start_failing_after) {
    return NULL;
  } else {
    return otter_malloc(mock->parent, size);
  }
}
otter_allocator_mock_malloc_failure *
initialize_mock_allocator(otter_allocator *parent, int start_failing_after) {
  otter_allocator_mock_malloc_failure *mock =
      otter_malloc(parent, sizeof(*mock));
  if (mock == NULL) {
    return NULL;
  }

  mock->mock_vtable = (otter_allocator_vtable){
      .malloc = otter_allocator_mock_malloc_failure_malloc_impl,
      .realloc = NULL,
      .free = parent->vtable->free,
  };
  mock->base.vtable = &mock->mock_vtable;
  mock->parent = parent;
  mock->num_malloc_calls = 0;
  mock->start_failing_after = start_failing_after;
  return mock;
}

OTTER_TEST(lexer_tokenize_malloc_failure_on_array_allocation) {
  otter_allocator_mock_malloc_failure *mock_allocator =
      initialize_mock_allocator(OTTER_TEST_ALLOCATOR, 1);

  otter_lexer *lexer =
      otter_lexer_create((otter_allocator *)mock_allocator, "(");
  size_t tokens_length = 0;
  otter_token **tokens = otter_lexer_tokenize(lexer, &tokens_length);
  OTTER_ASSERT(tokens == NULL);

  OTTER_TEST_END(otter_lexer_free(lexer);
                 otter_free(OTTER_TEST_ALLOCATOR, mock_allocator););
}

OTTER_TEST(lexer_tokenize_left_paren_malloc_failure) {
  otter_allocator_mock_malloc_failure *mock_allocator =
      initialize_mock_allocator(OTTER_TEST_ALLOCATOR, 2);

  otter_lexer *lexer =
      otter_lexer_create((otter_allocator *)mock_allocator, "(");
  size_t tokens_length = 0;
  otter_token **tokens = otter_lexer_tokenize(lexer, &tokens_length);
  OTTER_ASSERT(tokens == NULL);

  OTTER_TEST_END(otter_lexer_free(lexer);
                 otter_free(OTTER_TEST_ALLOCATOR, mock_allocator););
}

OTTER_TEST(lexer_tokenize_right_paren_malloc_failure) {
  otter_allocator_mock_malloc_failure *mock_allocator =
      initialize_mock_allocator(OTTER_TEST_ALLOCATOR, 2);

  otter_lexer *lexer =
      otter_lexer_create((otter_allocator *)mock_allocator, ")");
  size_t tokens_length = 0;
  otter_token **tokens = otter_lexer_tokenize(lexer, &tokens_length);
  OTTER_ASSERT(tokens == NULL);

  OTTER_TEST_END(otter_lexer_free(lexer);
                 otter_free(OTTER_TEST_ALLOCATOR, mock_allocator););
}

OTTER_TEST(lexer_tokenize_left_bracket_malloc_failure) {
  otter_allocator_mock_malloc_failure *mock_allocator =
      initialize_mock_allocator(OTTER_TEST_ALLOCATOR, 2);

  otter_lexer *lexer =
      otter_lexer_create((otter_allocator *)mock_allocator, "{");
  size_t tokens_length = 0;
  otter_token **tokens = otter_lexer_tokenize(lexer, &tokens_length);
  OTTER_ASSERT(tokens == NULL);

  OTTER_TEST_END(otter_lexer_free(lexer);
                 otter_free(OTTER_TEST_ALLOCATOR, mock_allocator););
}

OTTER_TEST(lexer_tokenize_right_bracket_malloc_failure) {
  otter_allocator_mock_malloc_failure *mock_allocator =
      initialize_mock_allocator(OTTER_TEST_ALLOCATOR, 2);

  otter_lexer *lexer =
      otter_lexer_create((otter_allocator *)mock_allocator, "}");
  size_t tokens_length = 0;
  otter_token **tokens = otter_lexer_tokenize(lexer, &tokens_length);
  OTTER_ASSERT(tokens == NULL);

  OTTER_TEST_END(otter_lexer_free(lexer);
                 otter_free(OTTER_TEST_ALLOCATOR, mock_allocator););
}

OTTER_TEST(lexer_tokenize_equals_malloc_failure) {
  otter_allocator_mock_malloc_failure *mock_allocator =
      initialize_mock_allocator(OTTER_TEST_ALLOCATOR, 2);

  otter_lexer *lexer =
      otter_lexer_create((otter_allocator *)mock_allocator, "==");
  size_t tokens_length = 0;
  otter_token **tokens = otter_lexer_tokenize(lexer, &tokens_length);
  OTTER_ASSERT(tokens == NULL);

  OTTER_TEST_END(otter_lexer_free(lexer);
                 otter_free(OTTER_TEST_ALLOCATOR, mock_allocator););
}

OTTER_TEST(lexer_tokenize_assignment_malloc_failure) {
  otter_allocator_mock_malloc_failure *mock_allocator =
      initialize_mock_allocator(OTTER_TEST_ALLOCATOR, 2);

  otter_lexer *lexer =
      otter_lexer_create((otter_allocator *)mock_allocator, "=");
  size_t tokens_length = 0;
  otter_token **tokens = otter_lexer_tokenize(lexer, &tokens_length);
  OTTER_ASSERT(tokens == NULL);

  OTTER_TEST_END(otter_lexer_free(lexer);
                 otter_free(OTTER_TEST_ALLOCATOR, mock_allocator););
}

OTTER_TEST(lexer_tokenize_minus_malloc_failure) {
  otter_allocator_mock_malloc_failure *mock_allocator =
      initialize_mock_allocator(OTTER_TEST_ALLOCATOR, 2);

  otter_lexer *lexer =
      otter_lexer_create((otter_allocator *)mock_allocator, "-");
  size_t tokens_length = 0;
  otter_token **tokens = otter_lexer_tokenize(lexer, &tokens_length);
  OTTER_ASSERT(tokens == NULL);

  OTTER_TEST_END(otter_lexer_free(lexer);
                 otter_free(OTTER_TEST_ALLOCATOR, mock_allocator););
}

OTTER_TEST(lexer_tokenize_integer_malloc_failure) {
  otter_allocator_mock_malloc_failure *mock_allocator =
      initialize_mock_allocator(OTTER_TEST_ALLOCATOR, 2);

  otter_lexer *lexer =
      otter_lexer_create((otter_allocator *)mock_allocator, "873");
  size_t tokens_length = 0;
  otter_token **tokens = otter_lexer_tokenize(lexer, &tokens_length);
  OTTER_ASSERT(tokens == NULL);

  OTTER_TEST_END(otter_lexer_free(lexer);
                 otter_free(OTTER_TEST_ALLOCATOR, mock_allocator););
}

OTTER_TEST(lexer_tokenize_var_malloc_failure) {
  otter_allocator_mock_malloc_failure *mock_allocator =
      initialize_mock_allocator(OTTER_TEST_ALLOCATOR, 2);

  otter_lexer *lexer =
      otter_lexer_create((otter_allocator *)mock_allocator, "var");
  size_t tokens_length = 0;
  otter_token **tokens = otter_lexer_tokenize(lexer, &tokens_length);
  OTTER_ASSERT(tokens == NULL);

  OTTER_TEST_END(otter_lexer_free(lexer);
                 otter_free(OTTER_TEST_ALLOCATOR, mock_allocator););
}

OTTER_TEST(lexer_tokenize_for_malloc_failure) {
  otter_allocator_mock_malloc_failure *mock_allocator =
      initialize_mock_allocator(OTTER_TEST_ALLOCATOR, 2);

  otter_lexer *lexer =
      otter_lexer_create((otter_allocator *)mock_allocator, "for");
  size_t tokens_length = 0;
  otter_token **tokens = otter_lexer_tokenize(lexer, &tokens_length);
  OTTER_ASSERT(tokens == NULL);

  OTTER_TEST_END(otter_lexer_free(lexer);
                 otter_free(OTTER_TEST_ALLOCATOR, mock_allocator););
}

OTTER_TEST(lexer_tokenize_if_malloc_failure) {
  otter_allocator_mock_malloc_failure *mock_allocator =
      initialize_mock_allocator(OTTER_TEST_ALLOCATOR, 2);

  otter_lexer *lexer =
      otter_lexer_create((otter_allocator *)mock_allocator, "if");
  size_t tokens_length = 0;
  otter_token **tokens = otter_lexer_tokenize(lexer, &tokens_length);
  OTTER_ASSERT(tokens == NULL);

  OTTER_TEST_END(otter_lexer_free(lexer);
                 otter_free(OTTER_TEST_ALLOCATOR, mock_allocator););
}

OTTER_TEST(lexer_tokenize_else_malloc_failure) {
  otter_allocator_mock_malloc_failure *mock_allocator =
      initialize_mock_allocator(OTTER_TEST_ALLOCATOR, 2);

  otter_lexer *lexer =
      otter_lexer_create((otter_allocator *)mock_allocator, "if");
  size_t tokens_length = 0;
  otter_token **tokens = otter_lexer_tokenize(lexer, &tokens_length);
  OTTER_ASSERT(tokens == NULL);

  OTTER_TEST_END(otter_lexer_free(lexer);
                 otter_free(OTTER_TEST_ALLOCATOR, mock_allocator););
}

OTTER_TEST(lexer_tokenize_identifier_malloc_failure) {
  otter_allocator_mock_malloc_failure *mock_allocator =
      initialize_mock_allocator(OTTER_TEST_ALLOCATOR, 2);

  otter_lexer *lexer =
      otter_lexer_create((otter_allocator *)mock_allocator, "ident1");
  size_t tokens_length = 0;
  otter_token **tokens = otter_lexer_tokenize(lexer, &tokens_length);
  OTTER_ASSERT(tokens == NULL);

  OTTER_TEST_END(otter_lexer_free(lexer);
                 otter_free(OTTER_TEST_ALLOCATOR, mock_allocator););
}

OTTER_TEST(lexer_tokenize_identifier_malloc_failure2) {
  otter_allocator_mock_malloc_failure *mock_allocator =
      initialize_mock_allocator(OTTER_TEST_ALLOCATOR, 3);

  otter_lexer *lexer =
      otter_lexer_create((otter_allocator *)mock_allocator, "ident1");
  size_t tokens_length = 0;
  otter_token **tokens = otter_lexer_tokenize(lexer, &tokens_length);
  OTTER_ASSERT(tokens == NULL);

  OTTER_TEST_END(otter_lexer_free(lexer);
                 otter_free(OTTER_TEST_ALLOCATOR, mock_allocator););
}
