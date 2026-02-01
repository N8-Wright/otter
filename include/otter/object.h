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
#ifndef OTTER_OBJECT_H_
#define OTTER_OBJECT_H_
#include <stddef.h>

typedef enum otter_object_default_type {
  OTTER_OBJECT_TYPE_NIL,
  OTTER_OBJECT_TYPE_BOOL,
  OTTER_OBJECT_TYPE_INTEGER,
  OTTER_OBJECT_TYPE_FLOAT,
  OTTER_OBJECT_TYPE_STRING,
} otter_object_default_type;

typedef struct otter_object {
  int type;
  struct otter_object *next; /* Intrusive linked list for GC */
} otter_object;

typedef struct otter_object_integer {
  otter_object base;
  int value;
} otter_object_integer;

typedef struct otter_object_float {
  otter_object base;
  double value;
} otter_object_float;

typedef struct otter_object_bool {
  otter_object base;
  int value;
} otter_object_bool;

typedef struct otter_object_string {
  otter_object base;
  char *data;
  size_t length;
} otter_object_string;

#endif /* OTTER_OBJECT_H_ */
