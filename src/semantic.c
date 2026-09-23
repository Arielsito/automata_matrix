#include "semantic.h"
#include "common.h"
#include "parser.h"


// AST
static void traverse(const AstNode*, TraversalOrder, AstVisitFn, void*);

static void traverse(const AstNode* node, TraversalOrder order, AstVisitFn visit, void* context) {
  if (node == NULL || visit == NULL) return;

  switch (node->type) {
    case NODE_LITERAL:
    case NODE_VARIABLE:
      visit(node, context);
      return;

    case NODE_BINARY:
      if (order == TRAVERSE_PREORDER) visit(node, context);
      traverse(node->as.binary.left, order, visit, context);
      if (order == TRAVERSE_INORDER) visit(node, context);
      traverse(node->as.binary.right, order, visit, context);
      if (order == TRAVERSE_POSTORDER) visit(node, context);
      return;

    case NODE_ASSIGN:
      if (order == TRAVERSE_PREORDER) visit(node, context);
      traverse(node->as.assign.target, order, visit, context);
      if (order == TRAVERSE_INORDER) visit(node, context);
      traverse(node->as.assign.value, order, visit, context);
      if (order == TRAVERSE_POSTORDER) visit(node, context);
      return;

    case NODE_UNARY:
      if (order == TRAVERSE_PREORDER) visit(node, context);
      if (order == TRAVERSE_INORDER && !node->as.unary.postfix) visit(node, context);
      traverse(node->as.unary.right, order, visit, context);
      if (order == TRAVERSE_INORDER && node->as.unary.postfix) visit(node, context);
      if (order == TRAVERSE_POSTORDER) visit(node, context);
      return;

    case NODE_INDEX:
      if (order == TRAVERSE_PREORDER) visit(node, context);
      traverse(node->as.index.object, order, visit, context);
      if (order == TRAVERSE_INORDER) visit(node, context);
      traverse(node->as.index.index, order, visit, context);
      if (order == TRAVERSE_POSTORDER) visit(node, context);
      return;

    case NODE_CALL:
      if (order == TRAVERSE_PREORDER) visit(node, context);
      traverse(node->as.call.callee, order, visit, context);
      if (order == TRAVERSE_INORDER) visit(node, context);
      for (i32 i = 0; i < node->as.call.arg_count; i++)
        traverse(node->as.call.args[i], order, visit, context);
      if (order == TRAVERSE_POSTORDER) visit(node, context);
      return;

    default:
      return;
  }
}

typedef bool (*SemanticRules)(AstNode *);

// validation functions
static bool validate_variable(AstNode *);
static bool validate_assign(AstNode *);
static bool validate_binary(AstNode *);
static bool validate_unary(AstNode *);
static bool validate_decl(AstNode *);

static SemanticRules rules[] = {
  [NODE_PROGRAM] = NULL,
  [NODE_LITERAL] = NULL,
  [NODE_VARIABLE] = validate_variable,
  [NODE_ASSIGN] = validate_assign,
  [NODE_BINARY] = validate_binary,
  [NODE_UNARY] = validate_unary,
  [NODE_STATEMENT] = NULL,
  [NODE_DECL] = validate_decl,
};

static bool validate_variable(AstNode *n) { return true; }

static bool validate_assign(AstNode *n) { return true; }

static bool validate_binary(AstNode *n) { return true; }

static bool validate_unary(AstNode *n) { return true; }

static bool validate_decl(AstNode *n) { return true; }

bool compile(AstNode *node) {
  if (node == NULL) return false;

  return true;
}

void traverse_expression(const AstNode *node, TraversalOrder order, AstVisitFn visit, void *context) {
  traverse(node, order, visit, context);
}
