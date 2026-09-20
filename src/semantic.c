#include "parser.h"

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
  if (node == NULL) return true;

  return true;
}
