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
#include "otter_allocator.h"

typedef enum otter_token_type {
  OTTER_TOKEN_LEFT_PAREN,    /* ( */
  OTTER_TOKEN_RIGHT_PAREN,   /* ) */
  OTTER_TOKEN_LEFT_BRACKET,  /* { */
  OTTER_TOKEN_RIGHT_BRACKET, /* } */
  OTTER_TOKEN_ASSIGNMENT,    /* = */
  OTTER_TOKEN_EQUALS,        /* == */
  OTTER_TOKEN_MINUS,         /* - */
  OTTER_TOKEN_VAR,           /* var */
  OTTER_TOKEN_FOR,           /* for */
  OTTER_TOKEN_IF,            /* if */
  OTTER_TOKEN_ELSE,          /* else */
  OTTER_TOKEN_INTEGER,       /* e.g., 1, 2, 3, ... */
  OTTER_TOKEN_FLOAT,         /* e.g., 1.123, ... */
  OTTER_TOKEN_IDENTIFIER,
} otter_token_type;

typedef struct otter_token {
  otter_token_type type;
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
#endif /* OTTER_TOKEN_H_ */
