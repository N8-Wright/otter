/*
  otter Copyright (C) 2026 Nathaniel Wright

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
#include "otter/string.h"
#include <assert.h>
#include <string.h>

#define FUDGE_FACTOR 16
static otter_string *otter_string_expand(otter_string *str, size_t capacity) {
  if (capacity < str->size) {
    return NULL;
  }

  str->capacity = capacity + FUDGE_FACTOR;

  void *result =
      otter_realloc(str->allocator, str, sizeof(*str) + str->capacity);
  if (result == NULL) {
    return NULL;
  }

  return result;
}

otter_string *otter_string_create(otter_allocator *allocator, const char *str_,
                                  size_t length) {
  if (allocator == NULL || str_ == NULL) {
    return NULL;
  }

  assert(strlen(str_) == length);
  otter_string *str = otter_malloc(allocator, sizeof(*str) + length + 1);
  if (str == NULL) {
    return NULL;
  }

  str->size = length + 1;
  str->capacity = length + 1;
  memcpy(str->data, str_, str->size);
  return str;
}

void otter_string_free(otter_string *str) { otter_free(str->allocator, str); }

void otter_string_append(otter_string **str, const char *append,
                         size_t length) {
  if (str == NULL || *str == NULL) {
    return;
  }

  if ((*str)->size + length < (*str)->capacity) {
    otter_string *expanded =
        otter_string_expand(*str, (*str)->capacity + length);
    if (expanded == NULL) {
      return;
    }

    *str = expanded;
  }

  (*str)->size += length;
  memcpy((*str)->data + (*str)->size - 1, append, length);
  (*str)->data[(*str)->size] = '\0';
}
