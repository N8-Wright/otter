#ifndef OTTER_LEXER_H_
#define OTTER_LEXER_H_
#include "otter_allocator.h"
#include <stddef.h>
typedef struct otter_lexer {
  otter_allocator *allocator;
  size_t index;
  size_t source_length;
  char *source;
} otter_lexer;

otter_lexer *otter_lexer_create_from_file(otter_allocator *allocator,
                                          const char *file);
otter_lexer *otter_lexer_create(otter_allocator *allocator, const char *source);
#endif /* OTTER_LEXER_H_ */
