#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>

#include "semantic.h"
#include "common.h"
#include "lexer.h"
#include "parser.h"
#include "arena.h"
#include "semantic_type.h"
#include "symtab.h"

/*
 * AST
 */
// ========================================================================================
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
// ========================================================================================

/*
 * Helpers
 */
// ========================================================================================
static bool is_integer_type(SemanticType type) {
  switch (type) {
    case SEM_TYPE_CHAR:
    case SEM_TYPE_SIGNED_CHAR:
    case SEM_TYPE_UNSIGNED_CHAR:
    case SEM_TYPE_SHORT:
    case SEM_TYPE_UNSIGNED_SHORT:
    case SEM_TYPE_INT:
    case SEM_TYPE_UNSIGNED_INT:
    case SEM_TYPE_LONG:
    case SEM_TYPE_UNSIGNED_LONG:
      return true;
    default:
      return false;
  }
}

static bool is_arithmetic_type(SemanticType type) {
  return is_integer_type(type) ||
    type == SEM_TYPE_FLOAT ||
    type == SEM_TYPE_DOUBLE ||
    type == SEM_TYPE_LONG_DOUBLE;
}

static TokenType compound_binary_operator(TokenType op) {
  switch (op) {
    case TOKEN_PLUS_EQUAL:     return TOKEN_PLUS;
    case TOKEN_MINUS_EQUAL:    return TOKEN_MINUS;
    case TOKEN_STAR_EQUAL:     return TOKEN_STAR;
    case TOKEN_SLASH_EQUAL:    return TOKEN_SLASH;
    case TOKEN_MOD_EQUAL:      return TOKEN_MOD;
    case TOKEN_AND_BITW_EQUAL: return TOKEN_AND_BITW;
    case TOKEN_OR_BITW_EQUAL:  return TOKEN_OR_BITW;
    case TOKEN_XOR_BITW_EQUAL: return TOKEN_XOR_BITW;
    case TOKEN_LSE:            return TOKEN_LEFT_SHIFT;
    case TOKEN_RSE:            return TOKEN_RIGHT_SHIFT;
    default:                   return TOKEN_ERROR;
  }
}

static bool can_assign_type(SemanticType target, SemanticType value) {
  return is_arithmetic_type(target) && is_arithmetic_type(value);
}

// ========================================================================================

/* 
 * Validation functions
 */
// ========================================================================================
static bool validate_literal(AstNode *);
static bool validate_variable(AstNode *);
static bool validate_assign(AstNode *);
static bool validate_binary(AstNode *);
static bool validate_unary(AstNode *);
static bool validate_call(AstNode *);
static bool validate_decl(AstNode *);
static bool validate_function(AstNode *);
// ========================================================================================

 
/*
 * Typecheck
 */
// ========================================================================================
static SemanticType integer_promotion(SemanticType);
static SemanticType common_arithmetic_type(SemanticType, SemanticType);
static SemanticType binary_result_type(TokenType, SemanticType, SemanticType);
// ========================================================================================

/*
 * Expressions
 */
// ========================================================================================
static void check_expr(AstNode *);

static SemanticType declared_type(TypeBase t) {
  if ((t.is_signed && t.is_unsigned) || (t.is_short && t.is_long))
    return SEM_TYPE_ERROR;
  switch (t.base) {
    case TOKEN_CHAR:
      if (t.is_short || t.is_long) return SEM_TYPE_ERROR;
      if (t.is_unsigned) return SEM_TYPE_UNSIGNED_CHAR;
      return t.is_signed ? SEM_TYPE_SIGNED_CHAR : SEM_TYPE_CHAR;
    case TOKEN_INT:
      if (t.is_short)
        return t.is_unsigned ? SEM_TYPE_UNSIGNED_SHORT : SEM_TYPE_SHORT;
      if (t.is_long)
        return t.is_unsigned ? SEM_TYPE_UNSIGNED_LONG : SEM_TYPE_LONG;
      return t.is_unsigned ? SEM_TYPE_UNSIGNED_INT : SEM_TYPE_INT;
    case TOKEN_DOUBLE:
      if (t.is_short || t.is_signed || t.is_unsigned) return SEM_TYPE_ERROR;
      return t.is_long ? SEM_TYPE_LONG_DOUBLE : SEM_TYPE_DOUBLE;
    case TOKEN_FLOAT:
    case TOKEN_VOID:
      if (t.is_short || t.is_long || t.is_signed || t.is_unsigned)
        return SEM_TYPE_ERROR;
      return t.base == TOKEN_FLOAT ? SEM_TYPE_FLOAT : SEM_TYPE_VOID;
    default:
      return SEM_TYPE_ERROR;
  }
}

static bool expression_error(AstNode *n) {
  n->semantic = (SemanticResult){ SEM_TYPE_ERROR, false, true };
  return false;
}
// ========================================================================================

/*
 * Quadruples
 */
// ========================================================================================
typedef struct quad_context {
  QuadrupleList *output;
  i32 next_temporary;
  i32 next_label;
} QuadContext;

static bool operand_from_leaf(const AstNode*, Operand*);
static Operand new_temporary(QuadContext*, SemanticType);
static bool emit_quad(QuadContext*, QuadOp, Operand, Operand, Operand, i32);
static bool convert_operand(QuadContext*, Operand*, SemanticType, i32);
static bool generate_binary_operation(QuadContext*, TokenType, Operand, Operand, i32, Operand*);
static bool generate_expression(const AstNode*, QuadContext*, Operand*);
static bool generate_node(const AstNode*, QuadContext*);
// ========================================================================================

/*
 * Rules
 */
// ========================================================================================
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
  [NODE_DECL] = validate_decl,
  [NODE_FUNCTION] = validate_function,
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
      return false;
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
  return failed ? expression_error(n) : true;
}

static bool validate_decl(AstNode *n) { return true; }

static bool validate_function(AstNode *n) {
  symtab_add_function(&symtab, n);
  return true;
}
// ========================================================================================

// typecheck
static SemanticType integer_promotion(SemanticType type) {
  switch (type) {
    case SEM_TYPE_SIGNED_CHAR:
    case SEM_TYPE_SHORT:
      return SEM_TYPE_INT;

    case SEM_TYPE_CHAR:
      return CHAR_MAX <= INT_MAX
        ? SEM_TYPE_INT
        : SEM_TYPE_UNSIGNED_INT;

    case SEM_TYPE_UNSIGNED_CHAR:
      return UCHAR_MAX <= INT_MAX
        ? SEM_TYPE_INT
        : SEM_TYPE_UNSIGNED_INT;

    case SEM_TYPE_UNSIGNED_SHORT:
      return USHRT_MAX <= INT_MAX
        ? SEM_TYPE_INT
        : SEM_TYPE_UNSIGNED_INT;

    default:
      return type;
  }
}

static SemanticType common_arithmetic_type(SemanticType left, SemanticType right) {
  if (!is_arithmetic_type(left) || !is_arithmetic_type(right)) return SEM_TYPE_ERROR;

  if (left == SEM_TYPE_LONG_DOUBLE || right == SEM_TYPE_LONG_DOUBLE) return SEM_TYPE_LONG_DOUBLE;
  if (left == SEM_TYPE_DOUBLE || right == SEM_TYPE_DOUBLE) return SEM_TYPE_DOUBLE;
  if (left == SEM_TYPE_FLOAT || right == SEM_TYPE_FLOAT) return SEM_TYPE_FLOAT;

  left = integer_promotion(left);
  right = integer_promotion(right);

  if (left == right) return left;

  if (left == SEM_TYPE_UNSIGNED_LONG || right == SEM_TYPE_UNSIGNED_LONG) return SEM_TYPE_UNSIGNED_LONG;
  if (left == SEM_TYPE_LONG || right == SEM_TYPE_LONG) {
    bool has_unsigned_int = left == SEM_TYPE_UNSIGNED_INT || right == SEM_TYPE_UNSIGNED_INT;
    if (has_unsigned_int && LONG_MAX <= UINT_MAX) return SEM_TYPE_UNSIGNED_LONG;
    return SEM_TYPE_LONG;
  }

  return SEM_TYPE_UNSIGNED_INT;
}

static SemanticType binary_result_type(TokenType op, SemanticType left, SemanticType right) {
  SemanticType result = SEM_TYPE_ERROR;

  switch (op) {
    case TOKEN_PLUS:
    case TOKEN_MINUS:
    case TOKEN_STAR:
    case TOKEN_SLASH:
      result = common_arithmetic_type(left, right);
      break;

    case TOKEN_MOD:
    case TOKEN_AND_BITW:
    case TOKEN_OR_BITW:
    case TOKEN_XOR_BITW:
      if (is_integer_type(left) && is_integer_type(right))
        result = common_arithmetic_type(left, right);
      break;

    case TOKEN_LESS:
    case TOKEN_LESS_EQUAL:
    case TOKEN_GREATER:
    case TOKEN_GREATER_EQUAL:
    case TOKEN_EQUAL_EQUAL:
    case TOKEN_NOT_EQUAL:
    case TOKEN_AND:
    case TOKEN_OR:
      if (is_arithmetic_type(left) && is_arithmetic_type(right))
        result = SEM_TYPE_INT;
      break;

    case TOKEN_LEFT_SHIFT:
    case TOKEN_RIGHT_SHIFT:
      if (is_integer_type(left) && is_integer_type(right))
        result = integer_promotion(left);
      break;

    default:
      break;
  }
  return result;
}

// quadruples
static bool operand_from_leaf(const AstNode *node, Operand *out) {
  if (node == NULL || out == NULL) return false;

  *out = (Operand){ .kind = OPERAND_NONE };

  if (node->semantic.has_error || node->semantic.type == SEM_TYPE_ERROR || node->semantic.type == SEM_TYPE_UNCHECKED)
    return false;

  Operand operand = {
    .type = node->semantic.type
  };

  switch (node->type) {
    case NODE_VARIABLE:
      if (node->as.variable.sym == NULL) return false;
      operand.kind = OPERAND_SYMBOL;
      operand.as.symbol = node->as.variable.sym;
      break;

    case NODE_LITERAL:
      operand.kind = OPERAND_CONSTANT;

      switch (node->as.literal.literalType) {
        case TOKEN_INTEGER_LITERAL:
          operand.as.constant.integer = node->as.literal.ival;
          break;
        case TOKEN_CHAR_LITERAL:
          operand.as.constant.integer = node->as.literal.cval;
          break;
        case TOKEN_DOUBLE_LITERAL:
          operand.as.constant.floating = node->as.literal.dval;
          break;
        default:
          return false;
      }
      break;

    default:
      return false;
  }

  *out = operand;
  return true;
}

static Operand new_temporary(QuadContext *ctx, SemanticType type) {
  Operand temp = {
    .kind = OPERAND_TEMPORARY,
    .type = type,
    .as.temporary_id = ctx->next_temporary++
  };

  return temp;
}

static bool emit_quad(QuadContext *ctx, QuadOp op, Operand arg1, Operand arg2, Operand result, i32 line) {
  QuadrupleList *output = ctx->output;
  if (output->count >= MAX_QUADRUPLES) return false;

  output->items[output->count++] = (Quadruple){ .op = op, arg1 = arg1, arg2 = arg2, result = result, line = line };
  return true;
}

static bool convert_operand(QuadContext *ctx, Operand *operand, SemanticType target_type, i32 line) {
  if (operand->type == target_type) return true;
  if (!can_assign_type(target_type, operand->type)) return false;

  Operand converted = new_temporary(ctx, target_type);
  Operand unused = { .kind = OPERAND_NONE };

  if (!emit_quad(ctx, QUAD_CONVERT, *operand, unused, converted, line)) return false;

  *operand = converted;
  return true;
}

static bool generate_binary_operation(QuadContext *ctx, TokenType token, Operand left, Operand right, i32 line, Operand *out) {
  QuadOp op;

  switch (token) {
    case TOKEN_PLUS:          op = QUAD_ADD;     break;
    case TOKEN_MINUS:         op = QUAD_SUB;     break;
    case TOKEN_STAR:          op = QUAD_MUL;     break;
    case TOKEN_SLASH:         op = QUAD_DIV;     break;
    case TOKEN_MOD:           op = QUAD_MOD;     break;
    case TOKEN_LESS:          op = QUAD_LT;      break;
    case TOKEN_LESS_EQUAL:    op = QUAD_LE;      break;
    case TOKEN_GREATER:       op = QUAD_GT;      break;
    case TOKEN_GREATER_EQUAL: op = QUAD_GE;      break;
    case TOKEN_EQUAL_EQUAL:   op = QUAD_EQ;      break;
    case TOKEN_NOT_EQUAL:     op = QUAD_NE;      break;
    case TOKEN_AND_BITW:      op = QUAD_BIT_AND; break;
    case TOKEN_OR_BITW:       op = QUAD_BIT_OR;  break;
    case TOKEN_XOR_BITW:      op = QUAD_BIT_XOR; break;
    case TOKEN_LEFT_SHIFT:    op = QUAD_SHL;     break;
    case TOKEN_RIGHT_SHIFT:   op = QUAD_SHR;     break;
    default: return false;
  }

  SemanticType type = binary_result_type(token, left.type, right.type);
  if (type == SEM_TYPE_ERROR) return false;

  bool comparison = 
    token == TOKEN_LESS ||
    token == TOKEN_LESS_EQUAL ||
    token == TOKEN_GREATER ||
    token == TOKEN_GREATER_EQUAL ||
    token == TOKEN_EQUAL_EQUAL ||
    token == TOKEN_NOT_EQUAL;
  bool shift = token == TOKEN_LEFT_SHIFT || token == TOKEN_RIGHT_SHIFT;

  SemanticType left_type = comparison ? common_arithmetic_type(left.type, right.type) : type;
  SemanticType right_type = shift ? integer_promotion(right.type) : left_type;
  if (!convert_operand(ctx, &left, left_type, line)) return false;
  if (!convert_operand(ctx, &right, right_type, line)) return false;

  Operand result = new_temporary(ctx, type);

  if (!emit_quad(ctx, op, left, right, result, line)) return false;
  *out = result;
  return true;
}

static bool generate_expression(const AstNode *n, QuadContext *ctx, Operand *out) {
  if (n == NULL || ctx == NULL || out == NULL) return false;

  *out = (Operand){ .kind = OPERAND_NONE };

  if (n->semantic.has_error || !is_arithmetic_type(n->semantic.type)) return false;

  // literals and variables
  if (n->type == NODE_LITERAL || n->type == NODE_VARIABLE) return operand_from_leaf(n, out);

  // unaries
  if (n->type == NODE_UNARY) {
    QuadOp op;

    if (n->as.unary.op == TOKEN_PLUS_PLUS || n->as.unary.op == TOKEN_MINUS_MINUS) {
      const AstNode *target = n->as.unary.right;
      if (target == NULL || target->type != NODE_VARIABLE) return false;
      
      Operand destination;
      if (!operand_from_leaf(target, &destination)) return false;

      Operand unused = { .kind = OPERAND_NONE };
      Operand previous = { .kind = OPERAND_NONE };

      if (n->as.unary.postfix) {
        previous = new_temporary(ctx, destination.type);
        if (!emit_quad(ctx, QUAD_COPY, destination, unused, previous, n->line)) return false;
      }

      Operand one = {
        .kind = OPERAND_CONSTANT,
        .type = SEM_TYPE_INT,
        .as.constant.integer = 1
      };

      TokenType binary = n->as.unary.op == TOKEN_PLUS_PLUS ? TOKEN_PLUS : TOKEN_MINUS;

      Operand updated;

      if (!generate_binary_operation(ctx, binary, destination, one, n->line, &updated)) return false;
      if (!convert_operand(ctx, &updated, destination.type, n->line)) return false;
      if (!emit_quad(ctx, QUAD_COPY, updated, unused, destination, n->line)) return false;
      *out = n->as.unary.postfix ? previous : updated;
      return true;
    }

    switch (n->as.unary.op) {
      case TOKEN_MINUS:
        op = QUAD_NEG;
        break;
      case TOKEN_NOT:
        op = QUAD_NOT;
        break;
      case TOKEN_NOT_BITW:
        op = QUAD_BIT_NOT;
        break;
      default:
        return false;
    }

    Operand operand;
    if (!generate_expression(n->as.unary.right, ctx, &operand)) return false;

    SemanticType type = n->semantic.type;
    if (n->as.unary.op != TOKEN_NOT) 
      if (!convert_operand(ctx, &operand, type, n->line)) return false;
    
    Operand result = new_temporary(ctx, type);
    Operand unused = { .kind = OPERAND_NONE };

    if (!emit_quad(ctx, op, operand, unused, result, n->line)) return false;
    *out = result;
    return true;
  }

  // assign
  if (n->type == NODE_ASSIGN) {
    const AstNode *target = n->as.assign.target;
    if (target == NULL || target->type != NODE_VARIABLE) return false;

    Operand destination, value;

    if (!operand_from_leaf(target, &destination)) return false;
    if (!generate_expression(n->as.assign.value, ctx, &value)) return false;

    if (n->as.assign.op != TOKEN_EQUAL) {
      TokenType binary = compound_binary_operator(n->as.assign.op);
      if (binary == TOKEN_ERROR) return false;
      if (!generate_binary_operation(ctx, binary, destination, value, n->line, &value)) return false;
    }

    if (!convert_operand(ctx, &value, destination.type, n->line)) return false;

    Operand unused = { .kind = OPERAND_NONE };

    if (!emit_quad(ctx, QUAD_COPY, value, unused, destination, n->line)) return false;
    *out = value;
    return true;
  }

  // binary (default)
  if (n->type != NODE_BINARY) return false;

  if (n->as.binary.op == TOKEN_AND || n->as.binary.op == TOKEN_OR) {
    bool is_and = n->as.binary.op == TOKEN_AND;
    QuadOp jump = is_and ? QUAD_JUMP_IF_ZERO : QUAD_JUMP_IF_NONZERO;
    Operand left;
    if (!generate_expression(n->as.binary.left, ctx, &left)) return false;

    Operand result = new_temporary(ctx, SEM_TYPE_INT);
    Operand end = { .kind = OPERAND_LABEL, .as.label_id = ctx->next_label++ };
    Operand unused = { .kind = OPERAND_NONE };
    Operand initial = { .kind = OPERAND_CONSTANT, .type = SEM_TYPE_INT, .as.constant.integer = is_and ? 0 : 1 };
    Operand alternative = initial;
    alternative.as.constant.integer = is_and ? 1 : 0;

    if (!emit_quad(ctx, QUAD_COPY, initial, unused, result, n->line)) return false;
    if (!emit_quad(ctx, jump, left, unused, end, n->line)) return false;

    Operand right;
    if (!generate_expression(n->as.binary.right, ctx, &right)) return false;
    if (!emit_quad(ctx, jump, right, unused, end, n->line)) return false;
    if (!emit_quad(ctx, QUAD_COPY, alternative, unused, result, n->line)) return false;
    if (!emit_quad(ctx, QUAD_LABEL, unused, unused, end, n->line)) return false;
    *out = result;
    return true;
  }

  Operand left, right;

  if (!generate_expression(n->as.binary.left, ctx, &left)) return false;
  if (!generate_expression(n->as.binary.right, ctx, &right)) return false;

  return generate_binary_operation(ctx, n->as.binary.op, left, right, n->line, out);
}

bool generate_expression_quads(const AstNode *expression, QuadrupleList *output, Operand *result) {
  if (expression == NULL || output == NULL || result == NULL) return false;
  
  *result = (Operand){ .kind = OPERAND_NONE };

  if (output->count != 0) return false;
  QuadContext ctx = { .output = output, .next_temporary = 0 };

  if (!generate_expression(expression, &ctx, result)) {
    output->count = 0;
    *result = (Operand){ .kind = OPERAND_NONE };
    return false;
  }

  return true;
}

static bool generate_node(const AstNode *node, QuadContext *ctx) {
  if (node == NULL) return false;

  switch (node->type) {
    case NODE_PROGRAM:
      for (i32 i = 0; i < node->as.program.count; i++)
        if (!generate_node(node->as.program.statements[i], ctx)) return false;
      return true;

    case NODE_STATEMENT: {
      Operand result;

      return generate_expression(node->as.statement.expression, ctx, &result);
    }

    case NODE_DECL:
      for (i32 i = 0; i < node->as.decl.count; i++) {
        const Declarator *declarator = &node->as.decl.declarators[i];
        if (declarator->init == NULL) continue;
        
        const Symbol *symbol = declarator->sym;
        if (symbol == NULL || symbol->semantic_type == SEM_TYPE_UNCHECKED || symbol->semantic_type == SEM_TYPE_ERROR)
          return false;

        Operand value;
        if (!generate_expression(declarator->init, ctx, &value)) return false;
        if (!convert_operand(ctx, &value, symbol->semantic_type, node->line)) return false;

        Operand destination = { .kind = OPERAND_SYMBOL, .type = symbol->semantic_type, .as.symbol = symbol };
        Operand unused = { .kind = OPERAND_NONE };

        if (!emit_quad(ctx, QUAD_COPY, value, unused, destination, node->line)) return false;
      }
      return true;

    default: return false;
  }
}

bool generate_quadruples(const AstNode *program, QuadrupleList *output) {
  if (program == NULL || output == NULL) return false;
  if (program->type != NODE_PROGRAM || output->count != 0) return false;

  QuadContext ctx = { .output = output, .next_temporary = 0 };

  if (!generate_node(program, &ctx)) { output->count = 0; return false; }
  return true;
}

// Symtab funcs
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

bool compile(AstNode *node) {
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
