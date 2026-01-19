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
#include "otter/bytecode.h"
#include <string.h>

#define OTTER_BYTECODE_HEADER_VERSION 1
typedef struct otter_bytecode_header {
  int version;
  int constants_length;
} otter_bytecode_header;

otter_bytecode *otter_bytecode_create(otter_allocator *allocator,
                                      const char *src, size_t source_length,
                                      otter_logger *logger) {
  OTTER_RETURN_IF_NULL(logger, allocator, NULL);
  OTTER_RETURN_IF_NULL(logger, src, NULL);
  OTTER_RETURN_IF_NULL(logger, logger, NULL);

  otter_bytecode_header header;
  if (source_length < sizeof(header)) {
    return NULL;
  }

  memcpy(&header, src, sizeof(header));
  if (header.version != OTTER_BYTECODE_HEADER_VERSION) {
    return NULL;
  }

  if (header.constants_length < 0) {
    return NULL;
  }

  otter_bytecode *bytecode = otter_malloc(allocator, sizeof(*bytecode));
  if (bytecode == NULL) {
    otter_log_critical(logger, "Failure to allocate %zd bytes",
                       sizeof(*bytecode));
    return NULL;
  }

  OTTER_ARRAY_INIT(bytecode, constants, allocator);
  if (bytecode->constants == NULL) {
    otter_log_critical(logger, "Failure to initialize array");
    goto failure;
  }

  size_t source_index = sizeof(header);
  for (int i = 0; i < header.constants_length; i++) {
    int constant_type;
    if (source_index + sizeof(constant_type) >= source_length) {
      otter_log_error(
          logger,
          "Unable to read %s from %s. The %s is at %zd when the %s is at %zd",
          OTTER_NAMEOF(constant_type), OTTER_NAMEOF(src),
          OTTER_NAMEOF(source_index), source_index, OTTER_NAMEOF(source_length),
          source_length);
      goto failure;
    }

    memcpy(&constant_type, &src[source_index], sizeof(constant_type));
    switch (constant_type) {
    case OTTER_OBJECT_TYPE_INTEGER: {
      source_index += sizeof(constant_type);
      int value;
      if (source_index + sizeof(value) >= source_index) {
        otter_log_error(
            logger,
            "Unable to read %s from %s. The %s is at %zd when the %s is at %zd",
            OTTER_NAMEOF(value), OTTER_NAMEOF(src), OTTER_NAMEOF(source_index),
            source_index, OTTER_NAMEOF(source_length), source_length);

        goto failure;
      }

      memcpy(&value, &src[source_index], sizeof(value));
      otter_object_integer *integer_obj =
          otter_malloc(allocator, sizeof(*integer_obj));
      integer_obj->base.type = OTTER_OBJECT_TYPE_INTEGER;
      integer_obj->value = value;
      if (!OTTER_ARRAY_APPEND(bytecode, constants, allocator,
                              (otter_object *)integer_obj)) {
        otter_log_critical(logger, "Unable to append %s to array %s",
                           OTTER_NAMEOF(integer_obj),
                           OTTER_NAMEOF(bytecode->constants));
        goto failure;
      }
    } break;
    default:
      goto failure;
    }
  }

failure:
  otter_bytecode_free(bytecode);
  return NULL;
}

void otter_bytecode_free(otter_bytecode *bytecode) {
  if (bytecode == NULL) {
    return;
  }

  otter_free(bytecode->allocator, bytecode->constants);
  otter_free(bytecode->allocator, bytecode);
}

OTTER_DEFINE_TRIVIAL_CLEANUP_FUNC(otter_bytecode *, otter_bytecode_free);
