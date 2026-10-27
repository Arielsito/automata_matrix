#ifndef IR_H
#define IR_H

#include "common.h"
#include "parser.h"
#include "symtab.h"
#include "semantic_type.h"

typedef enum operand_kind {
  OPERAND_NONE,
  OPERAND_SYMBOL,
  OPERAND_CONSTANT,
  OPERAND_TEMPORARY,
  OPERAND_LABEL,
} OperandKind;

struct Symbol;

typedef struct operand {
  OperandKind kind;
  SemanticType type;

  union {
    const struct Symbol *symbol;
    i32 temporary_id;
    i32 label_id;

    union {
      i32 integer;
      f64 floating;
    } constant;
  } as;
} Operand;

typedef enum quad_op {
  QUAD_ADD,
  QUAD_SUB,
  QUAD_NEG,
  QUAD_MUL,
  QUAD_DIV,
  QUAD_MOD,
  QUAD_NOT,
  QUAD_LT,
  QUAD_LE,
  QUAD_GT,
  QUAD_GE,
  QUAD_EQ,
  QUAD_NE,
  QUAD_JUMP_IF_ZERO,
  QUAD_JUMP_IF_NONZERO,
  QUAD_LABEL,
  QUAD_BIT_AND,
  QUAD_BIT_OR,
  QUAD_BIT_XOR,
  QUAD_BIT_NOT,
  QUAD_SHL,
  QUAD_SHR,
  QUAD_FUNC_BEGIN,
  QUAD_FUNC_END,
  QUAD_RETURN,
  QUAD_COPY,
  QUAD_CONVERT,
  QUAD_PARAM,
  QUAD_ARG,
  QUAD_CALL,
  //...
} QuadOp;

typedef struct quadruple {
  QuadOp op;
  Operand arg1;
  Operand arg2;
  Operand result;
  i32 line;
} Quadruple;

#define MAX_QUADRUPLES 256

typedef struct quadruple_list {
  Quadruple items[MAX_QUADRUPLES];
  i32 count;
} QuadrupleList;

bool generate_expression_quads(const AstNode*, QuadrupleList*, Operand*);
bool generate_quadruples(const AstNode*, QuadrupleList*);

#endif
