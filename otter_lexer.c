#include "otter_lexer.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>

otter_lexer *otter_lexer_create_from_file(otter_allocator *allocator,
                                          const char *file) {
  if (file == NULL) {
    return NULL;
  }

  FILE *f = fopen(file, "rb");
  if (f == NULL) {
    return NULL;
  }

  fseek(f, 0, SEEK_END);
  long size = ftell(f);
  if (size == -1) {
    fprintf(stderr, "Unable to read size of file: '%s'", strerror(errno));
    fclose(f);
    return NULL;
  }

  rewind(f);
  otter_lexer *lexer = otter_malloc(allocator, sizeof(*lexer));
  if (lexer == NULL) {
    fclose(f);
    return NULL;
  }

  lexer->allocator = allocator;
  lexer->index = 0;
  lexer->source_length = (size_t)size;
  lexer->source = otter_malloc(allocator, lexer->source_length + 1);
  if (lexer->source == NULL) {
    fclose(f);
    otter_free(allocator, lexer);
    return NULL;
  }

  size_t bytes_read = fread(lexer->source, 1, lexer->source_length, f);
  if (bytes_read != lexer->source_length) {
    int err = ferror(f);
    fprintf(stderr, "Unable to read entire file: '%s'", strerror(err));
    fclose(f);
    otter_free(allocator, lexer->source);
    otter_free(allocator, lexer);
    return NULL;
  }

  fclose(f);
  return lexer;
}

otter_lexer *otter_lexer_create(otter_allocator *allocator,
                                const char *source) {
  if (source == NULL) {
    return NULL;
  }

  otter_lexer *lexer = otter_malloc(allocator, sizeof(*lexer));
  if (lexer == NULL) {
    return NULL;
  }

  lexer->index = 0;
  lexer->source_length = strlen(source);
  lexer->source = otter_malloc(allocator, lexer->source_length + 1);
  if (lexer->source == NULL) {
    otter_free(allocator, lexer);
    return NULL;
  }

  memcpy(lexer->source, source, lexer->source_length);
  lexer->source[lexer->source_length] = '\0';

  return lexer;
}
