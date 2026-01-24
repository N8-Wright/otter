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

  size_t new_capacity = capacity + FUDGE_FACTOR;

  void *result =
      otter_realloc(str->allocator, str, sizeof(*str) + new_capacity);
  if (result == NULL) {
    return NULL;
  }

  otter_string *expanded = result;
  expanded->capacity = new_capacity;
  return expanded;
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

  str->allocator = allocator;
  str->size = length + 1;
  str->capacity = length + 1;
  memcpy(str->data, str_, length);
  str->data[length] = '\0';
  return str;
}

void otter_string_free(otter_string *str) {
  if (str == NULL) {
    return;
  }
  otter_free(str->allocator, str);
}

void otter_string_append(otter_string **str, const char *append,
                         size_t length) {
  if (str == NULL || *str == NULL || append == NULL) {
    return;
  }

  if ((*str)->size + length > (*str)->capacity) {
    otter_string *expanded =
        otter_string_expand(*str, (*str)->size + length - 1);
    if (expanded == NULL) {
      return;
    }

    *str = expanded;
  }

  memcpy((*str)->data + (*str)->size - 1, append, length);
  (*str)->size += length;
  (*str)->data[(*str)->size - 1] = '\0';
}

otter_string *otter_string_from_cstr(otter_allocator *allocator,
                                     const char *str) {
  if (str == NULL) {
    return NULL;
  }
  return otter_string_create(allocator, str, strlen(str));
}

void otter_string_append_cstr(otter_string **str, const char *append) {
  if (append == NULL) {
    return;
  }
  otter_string_append(str, append, strlen(append));
}

const char *otter_string_cstr(const otter_string *str) {
  if (str == NULL) {
    return NULL;
  }
  return str->data;
}

size_t otter_string_length(const otter_string *str) {
  if (str == NULL) {
    return 0;
  }
  return str->size > 0 ? str->size - 1 : 0;
}

void otter_string_clear(otter_string *str) {
  if (str == NULL) {
    return;
  }
  str->size = 1;
  str->data[0] = '\0';
}

int otter_string_compare(const otter_string *str1, const otter_string *str2) {
  if (str1 == NULL && str2 == NULL) {
    return 0;
  }
  if (str1 == NULL) {
    return -1;
  }
  if (str2 == NULL) {
    return 1;
  }

  if (str1->size != str2->size) {
    return str1->size < str2->size ? -1 : 1;
  }

  return memcmp(str1->data, str2->data, str1->size);
}

int otter_string_compare_cstr(const otter_string *str, const char *cstr) {
  if (str == NULL && cstr == NULL) {
    return 0;
  }
  if (str == NULL) {
    return -1;
  }
  if (cstr == NULL) {
    return 1;
  }

  return strcmp(str->data, cstr);
}
