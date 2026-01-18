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
#ifndef OTTER_CSTRING_H_
#define OTTER_CSTRING_H_
#include "otter_allocator.h"
#include <stdarg.h>
#include <stdbool.h>

char *otter_strndup(otter_allocator *allocator, const char *str, size_t len);
char *otter_strdup(otter_allocator *allocator, const char *str);
bool otter_vasprintf(otter_allocator *allocator, char **str, const char *fmt,
                     va_list args);
bool otter_asprintf(otter_allocator *allocator, char **str, const char *fmt,
                    ...);

#endif /* OTTER_CSTRING_H_ */
