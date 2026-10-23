#ifndef SEMANTIC_TYPE_H
#define SEMANTIC_TYPE_H

#include <stdbool.h>

typedef enum semantic_type {
  SEM_TYPE_UNCHECKED,
  SEM_TYPE_ERROR,
  SEM_TYPE_VOID,
  SEM_TYPE_CHAR,
  SEM_TYPE_SIGNED_CHAR,
  SEM_TYPE_UNSIGNED_CHAR,
  SEM_TYPE_SHORT,
  SEM_TYPE_UNSIGNED_SHORT,
  SEM_TYPE_INT,
  SEM_TYPE_UNSIGNED_INT,
  SEM_TYPE_LONG,
  SEM_TYPE_UNSIGNED_LONG,
  SEM_TYPE_FLOAT,
  SEM_TYPE_DOUBLE,
  SEM_TYPE_LONG_DOUBLE,
  SEM_TYPE_FUNCTION,
} SemanticType;

typedef struct semantic_result {
  SemanticType type;
  bool is_lvalue;
  bool has_error;
} SemanticResult;

#endif
