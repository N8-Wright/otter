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
#include "otter_logger.h"
#include "otter_cstring.h"
#include "otter_term_colors.h"
#include <assert.h>
#include <stdio.h>
#include <time.h>
typedef struct otter_logger_impl {
  otter_logger base;
  otter_allocator *allocator;
  otter_log_level log_level;
  size_t sinks_size;
  size_t sinks_capacity;
  otter_logger_sink_fn *sinks;
} otter_logger_impl;

static void otter_log_impl(otter_logger_impl *logger, otter_log_level log_level,
                           const char *fmt, va_list args) {
  time_t timestamp = time(NULL);
  char *message = NULL;
  bool formatted = otter_vasprintf(logger->allocator, &message, fmt, args);
  if (!formatted) {
    return;
  }

  for (size_t i = 0; i < logger->sinks_size; i++) {
    logger->sinks[i](log_level, timestamp, message);
  }

  otter_free(logger->allocator, message);
}

static void otter_log_debug_impl(otter_logger *logger_, const char *fmt,
                                 va_list args) {
  otter_logger_impl *logger = (otter_logger_impl *)logger_;
  if (logger->log_level >= OTTER_LOG_LEVEL_DEBUG) {
    otter_log_impl(logger, OTTER_LOG_LEVEL_DEBUG, fmt, args);
  }
}

static void otter_log_info_impl(otter_logger *logger_, const char *fmt,
                                va_list args) {
  otter_logger_impl *logger = (otter_logger_impl *)logger_;
  if (logger->log_level >= OTTER_LOG_LEVEL_INFO) {
    otter_log_impl(logger, OTTER_LOG_LEVEL_INFO, fmt, args);
  }
}

static void otter_log_warning_impl(otter_logger *logger_, const char *fmt,
                                   va_list args) {
  otter_logger_impl *logger = (otter_logger_impl *)logger_;
  if (logger->log_level >= OTTER_LOG_LEVEL_WARNING) {
    otter_log_impl(logger, OTTER_LOG_LEVEL_WARNING, fmt, args);
  }
}

static void otter_log_error_impl(otter_logger *logger_, const char *fmt,
                                 va_list args) {
  otter_logger_impl *logger = (otter_logger_impl *)logger_;
  if (logger->log_level >= OTTER_LOG_LEVEL_ERROR) {
    otter_log_impl(logger, OTTER_LOG_LEVEL_ERROR, fmt, args);
  }
}

static void otter_log_critical_impl(otter_logger *logger_, const char *fmt,
                                    va_list args) {
  otter_logger_impl *logger = (otter_logger_impl *)logger_;
  if (logger->log_level >= OTTER_LOG_LEVEL_CRITICAL) {
    otter_log_impl(logger, OTTER_LOG_LEVEL_CRITICAL, fmt, args);
  }
}

static otter_logger_vtable vtable = {
    .log_debug = otter_log_debug_impl,
    .log_info = otter_log_info_impl,
    .log_warning = otter_log_warning_impl,
    .log_error = otter_log_error_impl,
    .log_critical = otter_log_critical_impl,
};

static const char *otter_log_level_to_console_string(otter_log_level level) {
  switch (level) {
  case OTTER_LOG_LEVEL_CRITICAL:
    return OTTER_TERM_BRIGHT_RED("CRITICAL");
  case OTTER_LOG_LEVEL_ERROR:
    return OTTER_TERM_RED("ERROR");
  case OTTER_LOG_LEVEL_WARNING:
    return OTTER_TERM_YELLOW("WARNING");
  case OTTER_LOG_LEVEL_INFO:
    return OTTER_TERM_BLUE("INFO");
  case OTTER_LOG_LEVEL_DEBUG:
    return OTTER_TERM_MAGENTA("DEBUG");
  }

  assert(false && "uncreachable");
  return "";
}

const char *otter_log_level_to_string(otter_log_level level) {
  switch (level) {
  case OTTER_LOG_LEVEL_CRITICAL:
    return "CRITICAL";
  case OTTER_LOG_LEVEL_ERROR:
    return "ERROR";
  case OTTER_LOG_LEVEL_WARNING:
    return "WARNING";
  case OTTER_LOG_LEVEL_INFO:
    return "INFO";
  case OTTER_LOG_LEVEL_DEBUG:
    return "DEBUG";
  }

  assert(false && "uncreachable");
  return "";
}

otter_logger *otter_logger_create(otter_allocator *allocator,
                                  otter_log_level log_level) {
  otter_logger_impl *logger = otter_malloc(allocator, sizeof(*logger));
  if (logger == NULL) {
    return NULL;
  }

  logger->base.vtable = &vtable;
  logger->allocator = allocator;
  logger->log_level = log_level;
  logger->sinks_size = 0;
  logger->sinks_capacity = 2;
  logger->sinks = otter_malloc(allocator, sizeof(otter_logger_sink_fn) *
                                              logger->sinks_capacity);
  if (logger->sinks == NULL) {
    otter_free(allocator, logger);
    return NULL;
  }

  return (otter_logger *)logger;
}

void otter_logger_free(otter_logger *logger_) {
  otter_logger_impl *logger = (otter_logger_impl *)logger_;
  otter_free(logger->allocator, logger->sinks);
  otter_free(logger->allocator, logger);
}

void otter_logger_add_sink(otter_logger *logger_, otter_logger_sink_fn sink) {
  otter_logger_impl *logger = (otter_logger_impl *)logger_;
  if (logger->sinks_size == logger->sinks_capacity) {
    logger->sinks_capacity *= 2;
    void *result =
        otter_realloc(logger->allocator, logger->sinks,
                      sizeof(otter_logger_sink_fn) * logger->sinks_capacity);
    if (result == NULL) {
      return;
    }

    logger->sinks = result;
  }

  logger->sinks[logger->sinks_size++] = sink;
}

void otter_log_debug(otter_logger *logger, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  logger->vtable->log_debug(logger, fmt, args);
  va_end(args);
}

void otter_log_info(otter_logger *logger, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  logger->vtable->log_info(logger, fmt, args);
  va_end(args);
}

void otter_log_warning(otter_logger *logger, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  logger->vtable->log_warning(logger, fmt, args);
  va_end(args);
}

void otter_log_error(otter_logger *logger, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  logger->vtable->log_error(logger, fmt, args);
  va_end(args);
}

void otter_log_critical(otter_logger *logger, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  logger->vtable->log_critical(logger, fmt, args);
  va_end(args);
}

void otter_logger_console_sink(otter_log_level log_level, time_t timestamp,
                               const char *message) {
  char timestamp_string[84];
  struct tm time_info;
  gmtime_r(&timestamp, &time_info);
  strftime(timestamp_string, sizeof(timestamp_string), "%Y-%m-%d %H:%M:%S UTC",
           &time_info);
  printf("[%s] - %s - %s\n", timestamp_string,
         otter_log_level_to_console_string(log_level), message);
}
