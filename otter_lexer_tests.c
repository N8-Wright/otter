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
    otter_free(OTTER_TEST_ALLOCATOR, tokens[i]);
  } otter_free(OTTER_TEST_ALLOCATOR, tokens););
}
