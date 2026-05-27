#ifndef PARSER_H
#define PARSER_H

#include "./common.h"
#include "./lexer.h"

typedef enum nodetype {
  NODE_LITERAL,
  NODE_VARIABLE,
  NODE_ASSIGN,
  NODE_BINARY,
  NODE_UNARY,
  NODE_STATEMENT,
  NODE_IF,
  NODE_BLOCK,
  NODE_WHILE,
} NodeType;

typedef struct AstNode AstNode;

struct AstNode {
  NodeType type;
  i32 line;
  union {
    struct {
      TokenType literalType;
      union {
        i32 ival;
        f64 dval;
        char cval;
        char* sval;
      };
    } literal;

    struct {
      char* name;
    } variable;

    struct {
      char* target;
      TokenType op;
      AstNode* value;
    } assign;

    struct {
      AstNode* left;
      TokenType op;
      AstNode* right;
    } binary;

    struct {
      TokenType op;
      AstNode* right;
    } unary;

    struct {
      AstNode* expression;
    } statement;

    struct {
      AstNode* condition;
      AstNode* thenBranch;
      AstNode* elseBranch;
    } if_stmt;

    struct {
      AstNode** statements;
      i32 count;
      i32 capacity;
    } block;

    struct {
      AstNode* condition;
      AstNode* body;
    } while_stmt;
  } as;
};

bool compile(const char*);

#endif
