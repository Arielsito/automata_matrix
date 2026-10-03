#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "lexer.h"
#include "parser.h"

typedef enum traversal_order {
  TRAVERSE_PREORDER,
  TRAVERSE_INORDER,
  TRAVERSE_POSTORDER,
} TraversalOrder;

typedef struct quadruple {
  TokenType op;
  const char *arg1;
  const char *arg2;
  char result[16];
  const AstNode *node;
} Quadruple;

typedef void (*AstVisitFn)(const AstNode*, void*);

void traverse(const AstNode*, TraversalOrder, AstVisitFn, void*);
bool compile(AstNode *);

#endif
