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
#include <string.h>
/* Ensures that the variable being stringified _actually_ exists */
#define OTTER_NAMEOF(x) ((void)sizeof(&(x)), #x)
#define OTTER_NAMEOF_TYPE(t) ((void)sizeof((t *)NULL), #t)
#define OTTER_UNINITIALIZED_VAR(x) (void)sizeof(x)

#if defined(__GNUC__) || defined(__clang__)
#define OTTER_DIAGNOSTIC_PUSH() _Pragma("GCC diagnostic push")
#define OTTER_DIAGNOSTIC_IGNORE_MAYBE_UNINIT()                                 \
  _Pragma("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
#define OTTER_DIAGNOSTIC_POP() _Pragma("GCC diagnostic pop")
#else
/* Fallback: noops on unknown compilers */
#define OTTER_DIAGNOSTIC_PUSH()
#define OTTER_DIAGNOSTIC_IGNORE_MAYBE_UNINIT()
#define OTTER_DIAGNOSTIC_POP()
#endif

#define OTTER_DEFINE_TRIVIAL_CLEANUP_FUNC(type, func)                          \
  static inline void func##_p(type *p) {                                       \
    OTTER_DIAGNOSTIC_PUSH();                                                   \
    OTTER_DIAGNOSTIC_IGNORE_MAYBE_UNINIT();                                    \
    if (*p != NULL) {                                                          \
      func(*p);                                                                \
    }                                                                          \
    OTTER_DIAGNOSTIC_POP();                                                    \
  }                                                                            \
  struct otter_useless_struct_to_allow_trailing_semicolon

#define OTTER_RETURN_IF_NULL(logger, arg, ...)                                 \
  if ((arg) == NULL) {                                                         \
    otter_log_error(logger, "'%s' was NULL", OTTER_NAMEOF(arg));               \
    return __VA_ARGS__;                                                        \
  }

#define OTTER_CONCAT_IMPL(a, b) a##b
#define OTTER_CONCAT(a, b) OTTER_CONCAT_IMPL(a, b)
#define OTTER_UNIQUE_VARNAME(var)                                              \
  OTTER_CONCAT(otter_, OTTER_CONCAT(var, __LINE__))

#define OTTER_CLEANUP(func) __attribute__((cleanup(func)))

#if __has_attribute(__counted_by__)
#define OTTER_COUNTED_BY(field) __attribute__((counted_by(field)))
#else
#define OTTER_COUNTED_BY(field)
#endif
#endif /* OTTER_INC_H_ */
