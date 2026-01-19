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
#ifndef OTTER_BYTECODE_H_
#define OTTER_BYTECODE_H_
#include "allocator.h"
#include "array.h"
#include "inc.h"
#include "logger.h"
#include "object.h"
typedef struct otter_bytecode {
  otter_allocator *allocator;
  OTTER_ARRAY_DECLARE(otter_object *, constants);
  const char *instructions;
} otter_bytecode;

otter_bytecode *otter_bytecode_create(otter_allocator *allocator,
                                      const char *src, size_t source_length,
                                      otter_logger *logger);
void otter_bytecode_free(otter_bytecode *bytecode);
OTTER_DECLARE_TRIVIAL_CLEANUP_FUNC(otter_bytecode *, otter_bytecode_free);
#endif /* OTTER_BYTECODE_H_ */
