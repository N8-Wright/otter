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
