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
#ifndef OTTER_STRING_H_
#define OTTER_STRING_H_
#include "allocator.h"
#include "inc.h"
#include <stddef.h>
typedef struct otter_string otter_string;
otter_string *otter_string_create(otter_allocator *allocator, const char *str,
                                  size_t length);
otter_string *otter_string_format(otter_allocator *allocator,
                                  const char *format, ...);
otter_string *otter_string_from_cstr(otter_allocator *allocator,
                                     const char *str);
void otter_string_free(otter_string *str);
OTTER_DECLARE_TRIVIAL_CLEANUP_FUNC(otter_string *, otter_string_free);
size_t otter_string_capacity(const otter_string *str);
size_t otter_string_length(const otter_string *str);
void otter_string_append(otter_string **str, const char *append, size_t length);
void otter_string_append_cstr(otter_string **str, const char *append);
const char *otter_string_cstr(const otter_string *str);
void otter_string_clear(otter_string *str);
int otter_string_compare(const otter_string *str1, const otter_string *str2);
int otter_string_compare_cstr(const otter_string *str, const char *cstr);

#endif /* OTTER_STRING_H_ */
