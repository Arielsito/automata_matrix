#include "semantic.h"
#include "symtab.h"
#include "type_rules.h"
#include <stdarg.h>

/* AST */
void traverse(const AstNode* node, TraversalOrder order, AstVisitFn visit) {
  if (node == NULL || visit == NULL) return;

  switch (node->type) {
    case NODE_LITERAL:
    case NODE_VARIABLE:
      visit(node);
      return;

    case NODE_BINARY:
      if (order == TRAVERSE_PREORDER) visit(node);
      traverse(node->as.binary.left, order, visit);
      if (order == TRAVERSE_INORDER) visit(node);
      traverse(node->as.binary.right, order, visit);
      if (order == TRAVERSE_POSTORDER) visit(node);
      return;

    case NODE_ASSIGN:
      if (order == TRAVERSE_PREORDER) visit(node);
      traverse(node->as.assign.target, order, visit);
      if (order == TRAVERSE_INORDER) visit(node);
      traverse(node->as.assign.value, order, visit);
      if (order == TRAVERSE_POSTORDER) visit(node);
      return;

    case NODE_UNARY:
      if (order == TRAVERSE_PREORDER) visit(node);
      if (order == TRAVERSE_INORDER && !node->as.unary.postfix) visit(node);
      traverse(node->as.unary.right, order, visit);
      if (order == TRAVERSE_INORDER && node->as.unary.postfix) visit(node);
      if (order == TRAVERSE_POSTORDER) visit(node);
      return;

    case NODE_INDEX:
      if (order == TRAVERSE_PREORDER) visit(node);
      traverse(node->as.index.object, order, visit);
      if (order == TRAVERSE_INORDER) visit(node);
      traverse(node->as.index.index, order, visit);
      if (order == TRAVERSE_POSTORDER) visit(node);
      return;

    case NODE_CALL:
      if (order == TRAVERSE_PREORDER) visit(node);
      traverse(node->as.call.callee, order, visit);
      if (order == TRAVERSE_INORDER) visit(node);
      for (i32 i = 0; i < node->as.call.arg_count; i++)
        traverse(node->as.call.args[i], order, visit);
      if (order == TRAVERSE_POSTORDER) visit(node);
      return;

    default:
      return;
  }
}

static bool validate_literal(AstNode *);
static bool validate_variable(AstNode *);
static bool validate_assign(AstNode *);
static bool validate_binary(AstNode *);
static bool validate_unary(AstNode *);
static bool validate_call(AstNode *);
static bool validate_function(AstNode *);
static bool validate_return(AstNode *);

static void check_expr(AstNode *);

static bool expression_error(AstNode *n) {
  n->semantic = (SemanticResult){ SEM_TYPE_ERROR, false, true };
  return false;
}

/* functions */
static const AstNode *current_function;

static SemanticType parameter_type(const AstNode *parameter) {
  SemanticType type = declared_type(parameter->as.decl.type);
  if (!is_arithmetic_type(type)) return SEM_TYPE_ERROR;

  for (i32 i = 0; i < parameter->as.decl.count; i++) {
    const Declarator *declarator = &parameter->as.decl.declarators[i];

    for (const Declarator *part = declarator; part != NULL; part = part->paren)
      if (part->ptr_depth != 0 || part->arr_rank_counts != 0) return SEM_TYPE_ERROR;
  }

  return type;
}

/* Rules */
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

static SemanticRules rules[] = {
  [NODE_PROGRAM] = NULL,
  [NODE_LITERAL] = validate_literal,
  [NODE_VARIABLE] = validate_variable,
  [NODE_ASSIGN] = validate_assign,
  [NODE_BINARY] = validate_binary,
  [NODE_UNARY] = validate_unary,
  [NODE_STATEMENT] = NULL,
  [NODE_CALL] = validate_call,
  [NODE_FUNCTION] = validate_function,
  [NODE_RETURN] = validate_return,
};

static bool apply(AstNode *n) {
  if ((size_t)n->type >= sizeof(rules) / sizeof(rules[0])) return true;
  SemanticRules rule = rules[n->type];
  return rule == NULL || rule(n);
}

static bool validate_literal(AstNode *n) {
  n->semantic = (SemanticResult){ SEM_TYPE_UNCHECKED, false, false };

  switch (n->as.literal.literalType) {
    case TOKEN_INTEGER_LITERAL:
    case TOKEN_CHAR_LITERAL:
      n->semantic.type = SEM_TYPE_INT;
      return true;
    case TOKEN_DOUBLE_LITERAL:
      n->semantic.type = SEM_TYPE_DOUBLE;
      return true;
    case TOKEN_STRING_LITERAL: 
      sem_error(n->line, "strings literals are outside the current scalar subset.");
      return expression_error(n);
    default:
      sem_error(n->line, "unsupported literal kind.");
      return expression_error(n);
  }
}

static bool validate_variable(AstNode *n) {
  Symbol *s = symtab_lookup(&symtab, n->as.variable.name);
  n->as.variable.sym = s;
  if (s == NULL) {
    sem_error(n->line, "undeclared identifier '%s'.", n->as.variable.name);
    return expression_error(n);
  }
  if (s->semantic_type == SEM_TYPE_ERROR) return expression_error(n);
  n->semantic.type = s->semantic_type;
  n->semantic.is_lvalue = s->kind != SYM_FUNC;
  return true;
}

static bool validate_assign(AstNode *n) {
  SemanticResult target = n->as.assign.target->semantic;
  SemanticResult value = n->as.assign.value->semantic;
  if (target.has_error || value.has_error)
    return expression_error(n);

  if (!target.is_lvalue) {
    sem_error(n->line, "target not modifiable.");
    return expression_error(n);
  }

  if (!can_assign_type(target.type, value.type)) {
    sem_error(n->line, "type mismatch.");
    return expression_error(n);
  }

  if (n->as.assign.op != TOKEN_EQUAL) {
    TokenType op = compound_binary_operator(n->as.assign.op);

    if (op == TOKEN_ERROR) {
      sem_error(n->line, "assign operator not implemented.");
      return expression_error(n);
    }

    SemanticType result = binary_result_type(op, target.type, value.type);

    if (result == SEM_TYPE_ERROR) {
      sem_error(n->line, "type mismatch.");
      return expression_error(n);
    }
  }

  n->semantic = (SemanticResult){ target.type, false, false };
  return true; 
}

static bool validate_binary(AstNode *n) {
  SemanticResult left = n->as.binary.left->semantic;
  SemanticResult right = n->as.binary.right->semantic;
  if (left.has_error || right.has_error)
    return expression_error(n);

  SemanticType result = binary_result_type(n->as.binary.op, left.type, right.type);

  if (result == SEM_TYPE_ERROR) {
    sem_error(n->line, "invalid operands to binary expresion");
    return expression_error(n);
  }

  n->semantic = (SemanticResult){ result, false, false };
  return true; 
}

static bool validate_unary(AstNode *n) {
  SemanticResult operand = n->as.unary.right->semantic;

  if (operand.has_error)
    return expression_error(n);

  SemanticType result = SEM_TYPE_ERROR;

  switch (n->as.unary.op) {
    case TOKEN_MINUS:
      if (is_arithmetic_type(operand.type))
        result = integer_promotion(operand.type);
      break;

    case TOKEN_NOT:
      if (is_arithmetic_type(operand.type))
        result = SEM_TYPE_INT;
      break;

    case TOKEN_NOT_BITW:
      if (is_integer_type(operand.type))
        result = integer_promotion(operand.type);
      break;

    case TOKEN_PLUS_PLUS:
    case TOKEN_MINUS_MINUS:
      if (!is_arithmetic_type(operand.type) || !operand.is_lvalue) {
        sem_error(n->line, "increment/decrement requires an arithmetic object");
        return expression_error(n);
      }
      result = operand.type;
      break;

    default:
      sem_error(n->line, "unary operator not implemented.");
  }

  if (result == SEM_TYPE_ERROR) {
    sem_error(n->line, "invalid operands to unary expression.");
    return expression_error(n);
  }

  n->semantic = (SemanticResult){ result, false, false };
  return true;
}

static bool validate_call(AstNode *n) {
  bool failed = n->as.call.callee->semantic.has_error;
  for (i32 i = 0; i < n->as.call.arg_count; i++)
    if (n->as.call.args[i]->semantic.has_error) failed = true;

  if (failed) return expression_error(n);

  AstNode *callee = n->as.call.callee;

  if (callee->type != NODE_VARIABLE || callee->semantic.type != SEM_TYPE_FUNCTION) {
    sem_error(n->line, "called expression is not a function.");
    return expression_error(n);
  }

  Symbol *function = callee->as.variable.sym;
  const AstNode *prototype = function->prototype;
  n->as.call.prototype = prototype;

  if (prototype != NULL && n->as.call.arg_count != prototype->as.function.param_count) {
    sem_error(n->line, "wrong number of arguments to function '%s'.", function->name);
    return expression_error(n);
  }

  for (i32 i = 0; i < n->as.call.arg_count; i++) {
    SemanticType actual = n->as.call.args[i]->semantic.type;
    if (!is_arithmetic_type(actual)) {
      sem_error(n->line, "unsupported type for argument %d of '%s'.", i + 1, function->name);
      return expression_error(n);
    }

    if (prototype != NULL && !can_assign_type(parameter_type(prototype->as.function.params[i]), actual)) {
      sem_error(n->line, "incompatible type for argument %d of '%s'.", i + 1, function->name);
      return expression_error(n);
    }
  }

  n->semantic = (SemanticResult){ declared_type(function->type), false, false };
  return true;
}

static bool validate_function(AstNode *n) {
  if (n->as.function.ret_ptr_depth != 0) {
    sem_error(n->line, "pointer return types are outside the current scalar subset.");
    return false;
  }

  if (declared_type(n->as.function.type) == SEM_TYPE_ERROR) {
    sem_error(n->line, "invalid function return type.");
    return false;
  }

  for (i32 i = 0; i < n->as.function.param_count; i++) {
    AstNode *param = n->as.function.params[i];

    if (parameter_type(param) == SEM_TYPE_ERROR) {
      sem_error(param->line, "invalid or unsupported type for parameter %d of '%s'.", i + 1, n->as.function.name);
      return false;
    }

    const char *name = param->as.decl.count == 0 ? NULL : param->as.decl.declarators[0].name;
    if (name != NULL) {
      for (i32 j = 0; j < i; j++) {
        const AstNode *earlier = n->as.function.params[j];
        const char *earlier_name = earlier->as.decl.count == 0 ? NULL : earlier->as.decl.declarators[0].name;
        if (earlier_name != NULL && strcmp(name, earlier_name) == 0) {
          sem_error(param->line, "duplicate parameter name '%s' in function '%s'.", name, n->as.function.name);
          return false;
        }
      }
    }
  }

  Symbol *previous = symtab_lookup_local(&symtab, n->as.function.name);
  if (previous != NULL) {
    if (previous->kind != SYM_FUNC) {
      sem_error(n->line, "'%s' was already declared as an object.", n->as.function.name);
      return false;
    }

    SemanticType previous_return = declared_type(previous->type);
    SemanticType current_return = declared_type(n->as.function.type);

    if (previous_return != current_return) {
      sem_error(n->line, "conflicting return types for function '%s'.", n->as.function.name);
      return false;
    }

    const AstNode *previous_function = previous->prototype != NULL ? previous->prototype : previous->node;

    if (previous_function->as.function.has_prototype && n->as.function.has_prototype) {
      if (previous_function->as.function.param_count != n->as.function.param_count) {
        sem_error(n->line, "conflicting parameter counts for function '%s'.", n->as.function.name);
        return false;
      }

      for (i32 i = 0; i < n->as.function.param_count; i++) {
        SemanticType previous_param = parameter_type(previous_function->as.function.params[i]);
        SemanticType current_param = parameter_type(n->as.function.params[i]);

        if (previous_param != current_param) {
          sem_error(n->line, "conflicting types for parameter %d of function '%s'.", i + 1, n->as.function.name);
          return false;
        }
      }
    } else if (previous_function->as.function.has_prototype != n->as.function.has_prototype) {
      const AstNode *prototype = n->as.function.has_prototype ? n : previous_function;
      const AstNode *without_prototype = n->as.function.has_prototype ? previous_function : n;

      if (without_prototype->as.function.body != NULL && without_prototype->as.function.param_count != prototype->as.function.param_count) {
        sem_error(n->line, "conflicting parameter counts for function '%s'.", n->as.function.name);
        return false;
      }

      for (i32 i = 0; i < prototype->as.function.param_count; i++) {
        SemanticType type = parameter_type(prototype->as.function.params[i]);

        if (type != default_argument_promotion(type)) {
          sem_error(n->line, "parameter %d of '%s' conflicts with its declaration without prototype.", i + 1, n->as.function.name);
          return false;
        }
      }
    }
  }

  if (!symtab_add_function(&symtab, n)) {
    sem_error(n->line, "could not declare register function '%s'.", n->as.function.name);
    return false;
  }
  n->as.function.sym = symtab_lookup_local(&symtab, n->as.function.name);
  return true;
}

static bool validate_return(AstNode *n) {
  if (current_function == NULL) {
    sem_error(n->line, "return statement not within a function.");
    return false;
  }

  AstNode *value = n->as.return_stmt.value;
  SemanticType expected = declared_type(current_function->as.function.type);

  if (value == NULL) {
    if (expected != SEM_TYPE_VOID) {
      sem_error(n->line, "non-void function '%s' must return a value.", current_function->as.function.name);
      return false;
    }
    return true;
  }
  if (value->semantic.has_error) return false;
  if (expected == SEM_TYPE_VOID) {
    sem_error(n->line, "void function '%s' cannot return an expression.", current_function->as.function.name);
    return false;
  }
  if (!can_assign_type(expected, value->semantic.type)) {
    sem_error(n->line, "incompatible return type in function '%s'.", current_function->as.function.name);
    return false;
  }

  return true;
}

// Symtab
static bool declare_decl(AstNode *d, SymbolKind kind) {
  bool ok = true;
  SemanticType base = declared_type(d->as.decl.type);

  if (base == SEM_TYPE_ERROR || base == SEM_TYPE_VOID) {
    sem_error(d->line, "invalid type.");
    return false;
  }

  for (i32 i = 0; i < d->as.decl.count; i++) {
    Declarator *declarator = &d->as.decl.declarators[i];
    SemanticType type = base;

    for (const Declarator *part = declarator; part != NULL; part = part->paren) {
      if (part->ptr_depth != 0 || part->arr_rank_counts != 0) {
        sem_error(d->line, "pointers and arrays out of scope.");
        type = SEM_TYPE_ERROR;
        ok = false;
        break;
      }
    }

    Symbol *prev = NULL;
    Symbol *symbol = symtab_add_declarator(&symtab, d, i, kind, &prev);
    declarator->sym = symbol;

    if (symbol == NULL) {
      if (prev != NULL)
        sem_error(d->line, "identifier '%s' already declared in this scope.", prev->name);
      else 
        sem_error(d->line, "couldn't register declarator.");

      ok = false;
    } else
      symbol->semantic_type = type;

    AstNode *initializer = declarator->init;

    if (initializer != NULL) {
      check_expr(initializer);

      if (initializer->semantic.has_error)
        ok = false;
      else if (type != SEM_TYPE_ERROR && !can_assign_type(type, initializer->semantic.type)) {
        sem_error(d->line, "initializer incompatible with declared type.");
        ok = false;
      }
    }
  }

  return ok;
}

static void visit_expr(const AstNode *n) {
  AstNode *node = (AstNode *)n;
  node->semantic = (SemanticResult){ SEM_TYPE_UNCHECKED, false, false };
  if (node->type == NODE_INDEX) {
    sem_error(node->line, "array indexing is outside the current scalar subset.");
    expression_error(node);
    return;
  }
  apply(node);

  if (!node->semantic.has_error && node->semantic.type == SEM_TYPE_UNCHECKED) {
    sem_error(node->line, "expression type is unresolved or not implemented.");
    expression_error(node);
  }
}

static void check_expr(AstNode *e) {
  if (e != NULL && e->type == NODE_INIT_LIST) {
    e->semantic = (SemanticResult){ SEM_TYPE_UNCHECKED, false, false };
    for (i32 i = 0; i < e->as.init_list.count; i++) {
      AstNode *element = e->as.init_list.elements[i];
      check_expr(element);
      if (element->semantic.has_error) expression_error(e);
    }
    if (!e->semantic.has_error) {
      sem_error(e->line, "initializer lists are not supported yet.");
      expression_error(e);
    }
    return;
  }
  traverse(e, TRAVERSE_POSTORDER, visit_expr);
}

static void walk(AstNode *n);

static void walk_list(AstNode **v, i32 count) {
  for (i32 i = 0; i < count; i++) walk(v[i]);
}

static void walk_function(AstNode *fn) {
  if (!apply(fn)) return;
  if (fn->as.function.body == NULL) return;

  if (symtab_enter(&symtab, SCOPE_FUNCTION) == NULL) { sem_errors++; return; }

  const AstNode *previous_function = current_function;
  current_function = fn;

  for (i32 i = 0; i < fn->as.function.param_count; i++)
    declare_decl(fn->as.function.params[i], SYM_PARAM);

  AstNode *body = fn->as.function.body;
  walk_list(body->as.block.statements, body->as.block.count);

  current_function = previous_function;
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
      apply(n);
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

bool semantic_analysis(AstNode *node) {
  if (node == NULL) return false;

  if (sem_arena == NULL) {
    sem_arena = arena_create(MiB(1));
    if (sem_arena == NULL) { fprintf(stderr, "Error: Out of memory.\n"); return false; }
  } else arena_clear(sem_arena);

  if (!symtab_init(&symtab, sem_arena)) { fprintf(stderr, "Error: Out of memory.\n"); return false; }

  sem_errors = 0;
  current_function = NULL;
  walk(node);
  return sem_errors == 0;
}

void semantic_print_symtab(void) {
  symtab_print(&symtab, stdout);
}
