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
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

typedef struct otter_string {
  otter_allocator *allocator;
  size_t size;
  size_t capacity;
  char data[] OTTER_COUNTED_BY(size);
} otter_string;

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

otter_string *otter_string_format(otter_allocator *allocator,
                                  const char *format, ...) {
  if (allocator == NULL || format == NULL) {
    return NULL;
  }

  va_list args;
  va_list args_copy;
  va_start(args, format);
  va_copy(args_copy, args);
  const int needed = vsnprintf(NULL, 0, format, args);
  va_end(args);
  if (needed < 0) {
    va_end(args_copy);
    return NULL;
  }

  const size_t size_needed = (size_t)needed + 1;
  otter_string *str = otter_malloc(allocator, sizeof(*str) + size_needed);
  if (str == NULL) {
    va_end(args_copy);
    return NULL;
  }

  str->allocator = allocator;
  str->size = size_needed;
  str->capacity = size_needed;
  const int result = vsnprintf(str->data, size_needed, format, args_copy);
  va_end(args_copy);
  if (result < 0) {
    otter_free(allocator, str);
    return NULL;
  }

  return str;
}

otter_string *otter_string_copy(const otter_string *str) {
  if (str == NULL) {
    return NULL;
  }

  otter_allocator *allocator = str->allocator;
  otter_string *copy =
      otter_malloc(allocator, sizeof(otter_string) + str->size);
  if (copy == NULL) {
    return NULL;
  }

  copy->allocator = allocator;
  copy->size = str->size;
  copy->capacity = str->size;
  memcpy(copy->data, str->data, copy->size);
  return copy;
}

void otter_string_free(otter_string *str) {
  if (str == NULL) {
    return;
  }
  otter_free(str->allocator, str);
}
OTTER_DEFINE_TRIVIAL_CLEANUP_FUNC(otter_string *, otter_string_free);
size_t otter_string_capacity(const otter_string *str) { return str->capacity; }

size_t otter_string_length(const otter_string *str) {
  if (str == NULL) {
    return 0;
  }
  return str->size > 0 ? str->size - 1 : 0;
}

void otter_string_append(otter_string **str_, const char *append,
                         const size_t length) {
  if (str_ == NULL || *str_ == NULL || append == NULL) {
    return;
  }

  otter_string *str = *str_;
  if (str->size + length >= str->capacity) {
    otter_string *expanded = otter_string_expand(str, str->size + length);
    if (expanded == NULL) {
      return;
    }

    str = expanded;
  }

  assert(str->size > 0);
  const size_t offset = str->size - 1;
  str->size += length;
  memcpy(&str->data[offset], append, length);
  str->data[str->size - 1] = '\0';
  *str_ = str;
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

otter_string **otter_string_split(otter_allocator *allocator,
                                  const otter_string *str,
                                  const char *delimiters) {
  if (allocator == NULL || str == NULL || delimiters == NULL) {
    return NULL;
  }

  /* Create a working copy for tokenization */
  const char *cstr = otter_string_cstr(str);
  char *working_copy = otter_malloc(allocator, str->size);
  if (working_copy == NULL) {
    return NULL;
  }
  memcpy(working_copy, cstr, str->size);

  /* Count tokens first */
  size_t token_count = 0;
  char *token = strtok(working_copy, delimiters);
  while (token != NULL) {
    token_count++;
    token = strtok(NULL, delimiters);
  }

  /* Allocate result array (NULL-terminated) */
  otter_string **result =
      otter_malloc(allocator, (token_count + 1) * sizeof(otter_string *));
  if (result == NULL) {
    otter_free(allocator, working_copy);
    return NULL;
  }

  /* Reset working copy and tokenize again */
  memcpy(working_copy, cstr, str->size);
  size_t index = 0;
  token = strtok(working_copy, delimiters);
  while (token != NULL) {
    result[index] = otter_string_from_cstr(allocator, token);
    if (result[index] == NULL) {
      /* Cleanup on failure */
      for (size_t i = 0; i < index; i++) {
        otter_string_free(result[i]);
      }
      otter_free(allocator, result);
      otter_free(allocator, working_copy);
      return NULL;
    }
    index++;
    token = strtok(NULL, delimiters);
  }
  result[token_count] = NULL;

  otter_free(allocator, working_copy);
  return result;
}

char **otter_string_split_cstr(otter_allocator *allocator,
                               const otter_string *str,
                               const char *delimiters) {
  if (allocator == NULL || str == NULL || delimiters == NULL) {
    return NULL;
  }

  /* Create a working copy for tokenization */
  const char *cstr = otter_string_cstr(str);
  char *working_copy = otter_malloc(allocator, str->size);
  if (working_copy == NULL) {
    return NULL;
  }
  memcpy(working_copy, cstr, str->size);

  /* Count tokens first */
  size_t token_count = 0;
  char *token = strtok(working_copy, delimiters);
  while (token != NULL) {
    token_count++;
    token = strtok(NULL, delimiters);
  }

  /* Allocate result array (NULL-terminated) */
  char **result = otter_malloc(allocator, (token_count + 1) * sizeof(char *));
  if (result == NULL) {
    otter_free(allocator, working_copy);
    return NULL;
  }

  /* Reset working copy and tokenize again */
  memcpy(working_copy, cstr, str->size);
  size_t index = 0;
  token = strtok(working_copy, delimiters);
  while (token != NULL) {
    size_t token_len = strlen(token);
    result[index] = otter_malloc(allocator, token_len + 1);
    if (result[index] == NULL) {
      /* Cleanup on failure */
      for (size_t i = 0; i < index; i++) {
        otter_free(allocator, result[i]);
      }
      otter_free(allocator, result);
      otter_free(allocator, working_copy);
      return NULL;
    }
    memcpy(result[index], token, token_len + 1);
    index++;
    token = strtok(NULL, delimiters);
  }
  result[token_count] = NULL;

  otter_free(allocator, working_copy);
  return result;
}
