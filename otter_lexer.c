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
#include "otter_array.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>

otter_lexer *otter_lexer_create(otter_allocator *allocator,
                                const char *source) {
  if (allocator == NULL) {
    return NULL;
  }

  if (source == NULL) {
    return NULL;
  }

  otter_lexer *lexer = otter_malloc(allocator, sizeof(*lexer));
  if (lexer == NULL) {
    return NULL;
  }

  lexer->allocator = allocator;
  lexer->index = 0;
  lexer->source_length = strlen(source);
  lexer->source = source;
  return lexer;
}

void otter_lexer_free(otter_lexer *lexer) {
  otter_free(lexer->allocator, lexer);
}

typedef struct otter_token_array {
  OTTER_ARRAY_DECLARE(otter_token *, value);
} otter_token_array;

otter_token **otter_lexer_tokenize(otter_lexer *lexer, size_t *tokens_length) {
  if (lexer == NULL) {
    return NULL;
  }

  if (tokens_length == NULL) {
    return NULL;
  }

  otter_token_array tokens;
  OTTER_ARRAY_INIT(&tokens, value, lexer->allocator);
  if (tokens.value == NULL) {
    return NULL;
  }

  const char c = lexer->source[lexer->index];
  switch (c) {
  case '(': {
    otter_token *lparen = otter_malloc(lexer->allocator, sizeof(*lparen));
    if (lparen == NULL) {
      goto failure;
    }

    lparen->type = OTTER_TOKEN_LEFT_PAREN;
    if (!OTTER_ARRAY_APPEND(&tokens, value, lexer->allocator, lparen)) {
      otter_free(lexer->allocator, lparen);
      goto failure;
    }
  } break;
  }

  *tokens_length = OTTER_ARRAY_LENGTH(&tokens, value);
  return tokens.value;

failure:
  for (size_t i = 0; i < OTTER_ARRAY_LENGTH(&tokens, value); i++) {
    otter_token *token = OTTER_ARRAY_AT(&tokens, value, i);
    otter_free(lexer->allocator, token);
  }

  otter_free(lexer->allocator, tokens.value);

  return NULL;
}
