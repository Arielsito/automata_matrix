#ifndef SEMANTIC_H
#define SEMANTIC_H

#include "parser.h"

typedef enum traversal_order {
  TRAVERSE_PREORDER,
  TRAVERSE_INORDER,
  TRAVERSE_POSTORDER,
} TraversalOrder;

typedef void (*AstVisitFn)(const AstNode*);

void traverse(const AstNode*, TraversalOrder, AstVisitFn);
bool semantic_analysis(AstNode *);

void semantic_print_symtab(void);

#endif
