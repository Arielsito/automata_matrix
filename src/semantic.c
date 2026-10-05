#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "semantic.h"
#include "common.h"
#include "parser.h"
#include "arena.h"
#include "symtab.h"

// AST
void traverse(const AstNode* node, TraversalOrder order, AstVisitFn visit, void* context) {
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

static SymTab symtab;
static Arena *sem_arena;
static i32 sem_errors;

static void sem_error(i32 line, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  fprintf(stderr, "Semantic error at line %d: ", line);
  vfprintf(stderr, fmt, ap);
  fputc('\n', stderr);
  va_end(ap);
  sem_errors++;
}

static void check_expr(AstNode *);

// validation functions
static bool validate_variable(AstNode *);
static bool validate_assign(AstNode *);
static bool validate_binary(AstNode *);
static bool validate_unary(AstNode *);
static bool validate_call(AstNode *);
static bool validate_decl(AstNode *);
static bool validate_function(AstNode *);

static SemanticRules rules[] = {
  [NODE_PROGRAM] = NULL,
  [NODE_LITERAL] = NULL,
  [NODE_VARIABLE] = validate_variable,
  [NODE_ASSIGN] = validate_assign,
  [NODE_BINARY] = validate_binary,
  [NODE_UNARY] = validate_unary,
  [NODE_STATEMENT] = NULL,
  [NODE_CALL] = validate_call,
  [NODE_DECL] = validate_decl,
  [NODE_FUNCTION] = validate_function,
};

static bool apply(AstNode *n) {
  if ((size_t)n->type >= sizeof(rules) / sizeof(rules[0])) return true;
  SemanticRules rule = rules[n->type];
  return rule == NULL || rule(n);
}

static bool validate_variable(AstNode *n) { return true; }

static bool validate_assign(AstNode *n) { return true; }

static bool validate_binary(AstNode *n) { return true; }

static bool validate_unary(AstNode *n) { return true; }

static bool validate_call(AstNode *n) { return true; }

// Symtab funcs
static bool declare_decl(AstNode *d, SymbolKind kind) {
  symtab_add_decl(&symtab, d, kind);
  return true;
}

static bool validate_decl(AstNode *n) { return true; }

static bool validate_function(AstNode *n) {
  symtab_add_function(&symtab, n);
  return true;
}

static void visit_expr(const AstNode *n, void *ctx) {
  (void)ctx;
  apply((AstNode *)n);
}

static void check_expr(AstNode *e) {
  traverse(e, TRAVERSE_POSTORDER, visit_expr, NULL);
}

static void walk(AstNode *n);

static void walk_list(AstNode **v, i32 count) {
  for (i32 i = 0; i < count; i++) walk(v[i]);
}

static void walk_function(AstNode *fn) {
  apply(fn);
  if (fn->as.function.body == NULL) return;

  if (symtab_enter(&symtab, SCOPE_FUNCTION) == NULL) { sem_errors++; return; }
  for (i32 i = 0; i < fn->as.function.param_count; i++)
    declare_decl(fn->as.function.params[i], SYM_PARAM);
  walk(fn->as.function.body);
  symtab_exit(&symtab);
}

static void walk(AstNode *n) {
  if (n == NULL) return;
  switch (n->type) {
    case NODE_PROGRAM:
      walk_list(n->as.program.statements, n->as.program.count);
      return;
    case NODE_STATEMENT:
      check_expr(n->as.statement.expression);
      return;
    case NODE_DECL:
      declare_decl(n, SYM_VAR);
      return;
    case NODE_FUNCTION:
      walk_function(n);
      return;
    case NODE_BLOCK:
      if (symtab_enter(&symtab, SCOPE_BLOCK) == NULL) { sem_errors++; return; }
      walk_list(n->as.block.statements, n->as.block.count);
      symtab_exit(&symtab);
      return;
    case NODE_SWITCH:
      check_expr(n->as.switch_stmt.condition);
      if (symtab_enter(&symtab, SCOPE_BLOCK) == NULL) { sem_errors++; return; }
      walk_list(n->as.switch_stmt.cases, n->as.switch_stmt.count);
      symtab_exit(&symtab);
      return;
    case NODE_CASE:
      check_expr(n->as.case_stmt.expression);
      walk_list(n->as.case_stmt.statements, n->as.case_stmt.count);
      return;
    case NODE_RETURN:
      check_expr(n->as.return_stmt.value);
      return;
    case NODE_IF:
      check_expr(n->as.if_stmt.condition);
      walk(n->as.if_stmt.thenBranch);
      walk(n->as.if_stmt.elseBranch);
      return;
    case NODE_WHILE:
      check_expr(n->as.while_stmt.condition);
      walk(n->as.while_stmt.body);
      return;
    case NODE_DO_WHILE:
      walk(n->as.while_stmt.body);
      check_expr(n->as.while_stmt.condition);
      return;
    case NODE_FOR:
      walk(n->as.for_stmt.init);
      check_expr(n->as.for_stmt.condition);
      check_expr(n->as.for_stmt.update);
      walk(n->as.for_stmt.body);
      return;
    default:   // BREAK, CONTINUE, INIT_LIST
      return;
  }
}

bool compile(const AstNode *node) {
  if (node == NULL) return false;

  if (sem_arena == NULL) {
    sem_arena = arena_create(MiB(1));
    if (sem_arena == NULL) { fprintf(stderr, "Error: Out of memory.\n"); return false; }
  } else arena_clear(sem_arena);

  if (!symtab_init(&symtab, sem_arena)) { fprintf(stderr, "Error: Out of memory.\n"); return false; }

  sem_errors = 0;
  walk(node);
  return sem_errors == 0;
}

void semantic_print_symtab(void) {
  symtab_print(&symtab, stdout);
}
