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
#include "otter_cstring.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>

#define OTTER_LEXER_LINE_ZERO 1
#define OTTER_LEXER_COLUMN_ZERO 0
#define OTTER_LEXER_INCREMENT_POSITION(lexer)                                  \
  lexer->index++;                                                              \
  lexer->column++

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
  lexer->line = OTTER_LEXER_LINE_ZERO;
  lexer->column = OTTER_LEXER_COLUMN_ZERO;
  return lexer;
}

void otter_lexer_free(otter_lexer *lexer) {
  otter_free(lexer->allocator, lexer);
}

OTTER_DEFINE_TRIVIAL_CLEANUP_FUNC(otter_lexer *, otter_lexer_free);

typedef struct otter_token_array {
  OTTER_ARRAY_DECLARE(otter_token *, value);
} otter_token_array;

static bool otter_lexer_is_valid_identifier(const char c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
         (c >= '0' && c <= '9') || c == '-' || c == '_';
}

static bool otter_lexer_tokenize_string(otter_lexer *lexer,
                                        otter_token_array *tokens) {
  size_t begin = lexer->index;
  const int line = lexer->line;
  const int column = lexer->column;
  char c = lexer->source[lexer->index];
  OTTER_LEXER_INCREMENT_POSITION(lexer);

  while (lexer->index < lexer->source_length) {
    c = lexer->source[lexer->index];
    if (otter_lexer_is_valid_identifier(c)) {
      OTTER_LEXER_INCREMENT_POSITION(lexer);
    } else {
      break;
    }
  }

  size_t identifier_len = lexer->index - begin;
#define OTTER_APPEND_BASIC_TOKEN_OR_RETURN_FALSE(toktype)                      \
  do {                                                                         \
    otter_token *token = otter_malloc(lexer->allocator, sizeof(*token));       \
    if (token == NULL) {                                                       \
      return false;                                                            \
    }                                                                          \
    token->type = toktype;                                                     \
    token->line = line;                                                        \
    token->column = column;                                                    \
    if (!OTTER_ARRAY_APPEND(tokens, value, lexer->allocator, token)) {         \
      otter_free(lexer->allocator, token);                                     \
      return false;                                                            \
    }                                                                          \
  } while (0)

  if (0 == strncmp(&lexer->source[begin], "var", identifier_len)) {
    OTTER_APPEND_BASIC_TOKEN_OR_RETURN_FALSE(OTTER_TOKEN_VAR);
  } else if (0 == strncmp(&lexer->source[begin], "for", identifier_len)) {
    OTTER_APPEND_BASIC_TOKEN_OR_RETURN_FALSE(OTTER_TOKEN_FOR);
  } else if (0 == strncmp(&lexer->source[begin], "if", identifier_len)) {
    OTTER_APPEND_BASIC_TOKEN_OR_RETURN_FALSE(OTTER_TOKEN_IF);
  } else if (0 == strncmp(&lexer->source[begin], "else", identifier_len)) {
    OTTER_APPEND_BASIC_TOKEN_OR_RETURN_FALSE(OTTER_TOKEN_ELSE);
  } else {
    otter_token_identifier *ident =
        otter_malloc(lexer->allocator, sizeof(*ident));
    if (ident == NULL) {
      return false;
    }

    ident->base.type = OTTER_TOKEN_IDENTIFIER;
    ident->base.line = line;
    ident->base.column = column;
    ident->value =
        otter_strndup(lexer->allocator, &lexer->source[begin], identifier_len);
    if (ident->value == NULL) {
      otter_free(lexer->allocator, ident);
      return false;
    }

    if (!OTTER_ARRAY_APPEND(tokens, value, lexer->allocator,
                            (otter_token *)ident)) {
      return false;
    }
  }

  return true;
}

static bool otter_lexer_tokenize_integer(otter_lexer *lexer,
                                         otter_token_array *tokens,
                                         bool negate) {
  char c = lexer->source[lexer->index];
  const int line = lexer->line;
  const int column = lexer->column;
  OTTER_LEXER_INCREMENT_POSITION(lexer);
  int value = c - '0';

  while (lexer->index < lexer->source_length) {
    c = lexer->source[lexer->index];
    if (c >= '0' && c <= '9') {
      value = (value * 10) + (c - '0');
      OTTER_LEXER_INCREMENT_POSITION(lexer);
    } else {
      break;
    }
  }

  otter_token_integer *token = otter_malloc(lexer->allocator, sizeof(*token));
  if (token == NULL) {
    return false;
  }

  token->base.type = OTTER_TOKEN_INTEGER;
  token->base.line = line;
  token->base.column = column;
  token->value = value;
  if (negate) {
    token->value *= -1;
  }

  if (!OTTER_ARRAY_APPEND(tokens, value, lexer->allocator,
                          (otter_token *)token)) {
    otter_free(lexer->allocator, token);
    return false;
  }

  return true;
}

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

#define OTTER_APPEND_BASIC_TOKEN(toktype)                                      \
  do {                                                                         \
    otter_token *token = otter_malloc(lexer->allocator, sizeof(*token));       \
    if (token == NULL) {                                                       \
      goto failure;                                                            \
    }                                                                          \
    token->type = toktype;                                                     \
    token->line = lexer->line;                                                 \
    token->column = lexer->column;                                             \
    if (!OTTER_ARRAY_APPEND(&tokens, value, lexer->allocator, token)) {        \
      otter_free(lexer->allocator, token);                                     \
      goto failure;                                                            \
    }                                                                          \
  } while (0)

  while (lexer->index < lexer->source_length) {
    const char c = lexer->source[lexer->index];
    switch (c) {
    case '\n':
      lexer->line++;
      lexer->column = OTTER_LEXER_COLUMN_ZERO;
      /* fallthrough */
    case ' ':
    case '\t':
      /* Skip whitespace */
      break;
    case '(': {
      OTTER_APPEND_BASIC_TOKEN(OTTER_TOKEN_LEFT_PAREN);
    } break;
    case ')': {
      OTTER_APPEND_BASIC_TOKEN(OTTER_TOKEN_RIGHT_PAREN);
    } break;
    case '{': {
      OTTER_APPEND_BASIC_TOKEN(OTTER_TOKEN_LEFT_BRACKET);
    } break;
    case '}': {
      OTTER_APPEND_BASIC_TOKEN(OTTER_TOKEN_RIGHT_BRACKET);
    } break;
    case ';': {
      OTTER_APPEND_BASIC_TOKEN(OTTER_TOKEN_SEMICOLON);
    } break;
    case '*': {
      OTTER_APPEND_BASIC_TOKEN(OTTER_TOKEN_MULTIPLY);
    } break;
    case '/': {
      OTTER_APPEND_BASIC_TOKEN(OTTER_TOKEN_DIVIDE);
    } break;
    case '=': {
      if (lexer->index + 1 < lexer->source_length &&
          lexer->source[lexer->index + 1] == '=') {
        OTTER_LEXER_INCREMENT_POSITION(lexer);
        OTTER_APPEND_BASIC_TOKEN(OTTER_TOKEN_EQUALS);
      } else {
        OTTER_APPEND_BASIC_TOKEN(OTTER_TOKEN_ASSIGNMENT);
      }
    } break;
    case 'a':
    case 'b':
    case 'c':
    case 'd':
    case 'e':
    case 'f':
    case 'g':
    case 'h':
    case 'i':
    case 'j':
    case 'k':
    case 'l':
    case 'm':
    case 'n':
    case 'o':
    case 'p':
    case 'q':
    case 'r':
    case 's':
    case 't':
    case 'u':
    case 'v':
    case 'w':
    case 'x':
    case 'y':
    case 'z':
    case 'A':
    case 'B':
    case 'C':
    case 'D':
    case 'E':
    case 'F':
    case 'G':
    case 'H':
    case 'I':
    case 'J':
    case 'K':
    case 'L':
    case 'M':
    case 'N':
    case 'O':
    case 'P':
    case 'Q':
    case 'R':
    case 'S':
    case 'T':
    case 'U':
    case 'V':
    case 'W':
    case 'X':
    case 'Y':
    case 'Z': {
      if (!otter_lexer_tokenize_string(lexer, &tokens)) {
        goto failure;
      }
    } break;
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9': {
      if (!otter_lexer_tokenize_integer(lexer, &tokens, false)) {
        goto failure;
      }
    } break;
    case '-': {
      if (lexer->index + 1 < lexer->source_length) {
        if (lexer->source[lexer->index + 1] >= '1' &&
            lexer->source[lexer->index + 1] <= '9') {
          OTTER_LEXER_INCREMENT_POSITION(lexer);
          if (!otter_lexer_tokenize_integer(lexer, &tokens, true)) {
            goto failure;
          }
        } else if (lexer->source[lexer->index + 1] == '-') {
          OTTER_APPEND_BASIC_TOKEN(OTTER_TOKEN_DECREMENT);
          OTTER_LEXER_INCREMENT_POSITION(lexer);
        } else {
          OTTER_APPEND_BASIC_TOKEN(OTTER_TOKEN_MINUS);
        }
      } else {
        OTTER_APPEND_BASIC_TOKEN(OTTER_TOKEN_MINUS);
      }
    } break;
    case '+': {
      if (lexer->index + 1 < lexer->source_length) {
        if (lexer->source[lexer->index + 1] >= '1' &&
            lexer->source[lexer->index + 1] <= '9') {
          OTTER_LEXER_INCREMENT_POSITION(lexer);
          if (!otter_lexer_tokenize_integer(lexer, &tokens, false)) {
            goto failure;
          }
        } else if (lexer->source[lexer->index + 1] == '+') {
          OTTER_APPEND_BASIC_TOKEN(OTTER_TOKEN_INCREMENT);
          OTTER_LEXER_INCREMENT_POSITION(lexer);
        } else {
          OTTER_APPEND_BASIC_TOKEN(OTTER_TOKEN_PLUS);
        }
      } else {
        OTTER_APPEND_BASIC_TOKEN(OTTER_TOKEN_PLUS);
      }
    } break;
    }

    OTTER_LEXER_INCREMENT_POSITION(lexer);
  }

  *tokens_length = OTTER_ARRAY_LENGTH(&tokens, value);
  return tokens.value;

failure:
  OTTER_ARRAY_FOREACH(&tokens, value, otter_free, lexer->allocator);
  otter_free(lexer->allocator, tokens.value);

  return NULL;
}
