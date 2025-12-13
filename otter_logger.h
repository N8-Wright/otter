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
#ifndef OTTER_LOGGER_H_
#define OTTER_LOGGER_H_
#include "otter_allocator.h"
#include "otter_inc.h"
#include <stdarg.h>
#include <stddef.h>
#include <time.h>
typedef struct otter_logger otter_logger;
typedef struct otter_logger_vtable {
  void (*log_debug)(otter_logger *, const char *fmt, va_list args);
  void (*log_info)(otter_logger *, const char *fmt, va_list args);
  void (*log_warning)(otter_logger *, const char *fmt, va_list args);
  void (*log_error)(otter_logger *, const char *fmt, va_list args);
  void (*log_critical)(otter_logger *, const char *fmt, va_list args);
} otter_logger_vtable;

typedef enum otter_log_level {
  OTTER_LOG_LEVEL_CRITICAL = 0,
  OTTER_LOG_LEVEL_ERROR = 1,
  OTTER_LOG_LEVEL_WARNING = 2,
  OTTER_LOG_LEVEL_INFO = 3,
  OTTER_LOG_LEVEL_DEBUG = 4,
} otter_log_level;

typedef void (*otter_logger_sink_fn)(otter_log_level log_level,
                                     time_t timestamp, const char *);
struct otter_logger {
  otter_logger_vtable *vtable;
};

const char *otter_log_level_to_string(otter_log_level level);
otter_logger *otter_logger_create(otter_allocator *allocator,
                                  otter_log_level log_level);
void otter_logger_free(otter_logger *logger);
OTTER_DEFINE_TRIVIAL_CLEANUP_FUNC(otter_logger *, otter_logger_free);
void otter_logger_add_sink(otter_logger *logger, otter_logger_sink_fn sink);
void otter_log_debug(otter_logger *logger, const char *fmt, ...);
void otter_log_info(otter_logger *logger, const char *fmt, ...);
void otter_log_warning(otter_logger *logger, const char *fmt, ...);
void otter_log_error(otter_logger *logger, const char *fmt, ...);
void otter_log_critical(otter_logger *logger, const char *fmt, ...);

void otter_logger_console_sink(otter_log_level log_level, time_t timestamp,
                               const char *message);
#endif /* OTTER_LOGGER_H_ */
