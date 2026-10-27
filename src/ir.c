#include "ir.h"
#include "type_rules.h"

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
