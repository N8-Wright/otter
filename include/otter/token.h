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
#ifndef OTTER_TOKEN_H_
#define OTTER_TOKEN_H_
#include "allocator.h"

#define OTTER_TOKEN_TYPES                                                      \
  X(OTTER_TOKEN_LEFT_PAREN, "(")                                               \
  X(OTTER_TOKEN_RIGHT_PAREN, ")")                                              \
  X(OTTER_TOKEN_LEFT_BRACKET, "{")                                             \
  X(OTTER_TOKEN_RIGHT_BRACKET, "}")                                            \
  X(OTTER_TOKEN_ASSIGNMENT, "=")                                               \
  X(OTTER_TOKEN_EQUALS, "==")                                                  \
  X(OTTER_TOKEN_MINUS, "-")                                                    \
  X(OTTER_TOKEN_DECREMENT, "--")                                               \
  X(OTTER_TOKEN_PLUS, "+")                                                     \
  X(OTTER_TOKEN_INCREMENT, "++")                                               \
  X(OTTER_TOKEN_MULTIPLY, "*")                                                 \
  X(OTTER_TOKEN_DIVIDE, "/")                                                   \
  X(OTTER_TOKEN_SEMICOLON, ";")                                                \
  X(OTTER_TOKEN_VAR, "var")                                                    \
  X(OTTER_TOKEN_FOR, "for")                                                    \
  X(OTTER_TOKEN_IF, "if")                                                      \
  X(OTTER_TOKEN_ELSE, "else")                                                  \
  X(OTTER_TOKEN_INTEGER, "[integer]")                                          \
  X(OTTER_TOKEN_FLOAT, "[float]")                                              \
  X(OTTER_TOKEN_DEFINE_FUNCTION, "defn")                                       \
  X(OTTER_TOKEN_CALL_FUNCTION, "callfn")                                       \
  X(OTTER_TOKEN_IDENTIFIER, "[identifier]")

#define X(arg, str) arg,
typedef enum otter_token_type {
  OTTER_TOKEN_TYPES OTTER_TOKEN_COUNT_
} otter_token_type;
#undef X

typedef struct otter_token {
  otter_token_type type;
  int line;
  int column;
} otter_token;

typedef struct otter_token_identifier {
  otter_token base;
  char *value;
} otter_token_identifier;

typedef struct otter_token_integer {
  otter_token base;
  int value;
} otter_token_integer;

typedef struct otter_token_float {
  otter_token base;
  float value;
} otter_token_float;

void otter_token_free(otter_allocator *allocator, otter_token *token);
const char *otter_token_str(otter_token_type token_type);
#endif /* OTTER_TOKEN_H_ */
