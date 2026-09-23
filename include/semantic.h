#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "parser.h"

typedef enum traversal_order {
  TRAVERSE_PREORDER,
  TRAVERSE_INORDER,
  TRAVERSE_POSTORDER,
} TraversalOrder;

typedef void (*AstVisitFn)(const AstNode*, void*);

void traverse_expression(const AstNode*, TraversalOrder, AstVisitFn, void*);
bool compile(AstNode *);

#endif
