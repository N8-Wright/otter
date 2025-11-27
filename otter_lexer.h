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
#ifndef OTTER_LEXER_H_
#define OTTER_LEXER_H_
#include "otter_allocator.h"
#include <stddef.h>
typedef struct otter_lexer {
  otter_allocator *allocator;
  size_t index;
  size_t source_length;
  char *source;
} otter_lexer;

otter_lexer *otter_lexer_create_from_file(otter_allocator *allocator,
                                          const char *file);
otter_lexer *otter_lexer_create(otter_allocator *allocator, const char *source);
#endif /* OTTER_LEXER_H_ */
