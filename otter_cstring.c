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
#include "otter_cstring.h"
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

char *otter_strndup(otter_allocator *allocator, const char *str, size_t len) {
  char *dup = otter_malloc(allocator, len + 1);
  if (dup == NULL) {
    return NULL;
  }

  memcpy(dup, str, len);
  dup[len] = '\0';
  return dup;
}

char *otter_strdup(otter_allocator *allocator, const char *str) {
  if (allocator == NULL) {
    return NULL;
  }

  if (str == NULL) {
    return NULL;
  }

  return otter_strndup(allocator, str, strlen(str));
}

bool otter_vasprintf(otter_allocator *allocator, char **str, const char *fmt,
                     va_list args) {
  if (allocator == NULL) {
    return false;
  }

  if (str == NULL) {
    return false;
  }

  if (fmt == NULL) {
    return false;
  }

  va_list args_copy;
  va_copy(args_copy, args);

  int needed = vsnprintf(NULL, 0, fmt, args);

  if (needed < 0) {
    return false;
  }

  size_t size_needed = (size_t)needed + 1;
  *str = otter_malloc(allocator, size_needed);
  if (*str == NULL) {
    return false;
  }

  int result = vsnprintf(*str, size_needed, fmt, args_copy);
  if (result < 0) {
    otter_free(allocator, *str);
    return false;
  }

  va_end(args_copy);
  return true;
}

bool otter_asprintf(otter_allocator *allocator, char **str, const char *fmt,
                    ...) {
  if (allocator == NULL) {
    return false;
  }

  if (str == NULL) {
    return false;
  }

  if (fmt == NULL) {
    return false;
  }

  va_list args;
  va_start(args, fmt);
  otter_vasprintf(allocator, str, fmt, args);
  va_end(args);
  return true;
}
