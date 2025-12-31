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
#ifndef OTTER_INC_H_
#define OTTER_INC_H_
#include <stddef.h>
/* Ensures that the variable being stringified _actually_ exists */
#define OTTER_NAMEOF(x) ((void)sizeof(&(x)), #x)
#define OTTER_NAMEOF_TYPE(t) ((void)sizeof((t *)NULL), #t)
#define OTTER_DEFINE_TRIVIAL_CLEANUP_FUNC(type, func)                          \
  static inline void func##_p(type *p) {                                       \
    if (*p) {                                                                  \
      func(*p);                                                                \
    }                                                                          \
  }                                                                            \
  struct otter_useless_struct_to_allow_trailing_semicolon

#define OTTER_RETURN_IF_NULL(logger, arg, retval)                              \
  if ((arg) == NULL) {                                                         \
    otter_log_error(logger, "'%s' was NULL", OTTER_NAMEOF(arg));               \
    return retval;                                                             \
  }

#define OTTER_CONCAT_IMPL(a, b) a##b
#define OTTER_CONCAT(a, b) OTTER_CONCAT_IMPL(a, b)
#define OTTER_UNIQUE_VARNAME(var)                                              \
  OTTER_CONCAT(otter_, OTTER_CONCAT(var, __LINE__))

#define OTTER_CLEANUP(func) __attribute__((cleanup(func)))
#define OTTER_COUNTED_BY(field) __attribute__((counted_by(field)))
#endif /* OTTER_INC_H_ */
