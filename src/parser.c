#include "../include/common.h"
#include "../include/lexer.h"
#include <stdio.h>
#include <string.h>
#include "../include/parser.h"
#include "../include/arena.h"

// presedence
typedef enum presedence {
  PREC_NONE,
  PREC_ASSIGN,
  PREC_OR,
  PREC_AND,
  PREC_BITW,
  PREC_EQUAL,
  PREC_COMPARE,
  PREC_SHIFT,
  PREC_TERM,
  PREC_FACTOR,
  PREC_UNARY,
  PREC_CALL
} Presedence;

typedef AstNode* (*PrefixFn)();
typedef AstNode* (*InfixFn)(AstNode* left);

typedef struct {
  PrefixFn prefix;
  InfixFn infix;
  Presedence p;
} ParseRule;

// AST functions
static AstNode* make_node(Arena *arena, NodeType type, i32 line) {
  AstNode* n = PUSH_STRUCT(arena, AstNode);
  // AstNode* n = malloc(sizeof(AstNode));
  n->type = type;
  n->line = line;
  return n;
}

// parse functions
static AstNode* parse_statement();
static AstNode* parse_block();
static AstNode* parse_if();
static AstNode* parse_while();
static AstNode* parse_expression_stmt();

// pratt parser for math expressions
static AstNode* parse_expression();
static AstNode* parse_presedence(Presedence);
static AstNode* number();
static AstNode* variable();
static AstNode* grouping();
static AstNode* unary();
static AstNode* binary(AstNode*);
static AstNode* assign(AstNode*);

static ParseRule rules[] = {
  [TOKEN_INTEGER_LITERAL] = { number, NULL, PREC_NONE },
  [TOKEN_DOUBLE_LITERAL] = { number, NULL, PREC_NONE },
  [TOKEN_IDENTIFIER] = { variable, NULL, PREC_NONE },
  [TOKEN_LEFT_PAREN] = { grouping, NULL, PREC_NONE },

  [TOKEN_NOT] = { unary, NULL, PREC_NONE },
  [TOKEN_NOT_BITW] = { unary, NULL, PREC_NONE },
  [TOKEN_MINUS] = { unary, binary, PREC_TERM },

  [TOKEN_EQUAL] = { NULL, assign, PREC_ASSIGN },
  [TOKEN_PLUS_EQUAL] = { NULL, assign, PREC_ASSIGN },
  [TOKEN_MINUS_EQUAL] = { NULL, assign, PREC_ASSIGN },
  [TOKEN_STAR_EQUAL] = { NULL, assign, PREC_ASSIGN },
  [TOKEN_SLASH_EQUAL] = { NULL, assign, PREC_ASSIGN },
  [TOKEN_MOD_EQUAL] = { NULL, assign, PREC_ASSIGN },

  [TOKEN_PLUS] = { NULL, binary, PREC_TERM },
  [TOKEN_STAR] = { NULL, binary, PREC_FACTOR },
  [TOKEN_SLASH] = { NULL, binary, PREC_FACTOR },
  [TOKEN_MOD] = { NULL, binary, PREC_FACTOR },

  [TOKEN_EQUAL_EQUAL] = { NULL, binary, PREC_EQUAL },
  [TOKEN_NOT_EQUAL] = { NULL, binary, PREC_EQUAL },
  [TOKEN_LESS] = { NULL, binary, PREC_COMPARE },
  [TOKEN_LESS_EQUAL] = { NULL, binary, PREC_COMPARE },
  [TOKEN_GREATER] = { NULL, binary, PREC_COMPARE },
  [TOKEN_GREATER_EQUAL] = { NULL, binary, PREC_COMPARE },

  [TOKEN_AND] = { NULL, binary, PREC_AND },
  [TOKEN_OR] = { NULL, binary, PREC_OR },
};

static ParseRule *get_rule(TokenType t) {
  return &rules[t];
}

typedef struct parser {
  Token current;
  Token previous;
  bool hadError;
  bool panicMode;
  const char* source;
} Parser;

static Parser parser;
Arena *perm_arena; 

static void advance();
static void consume(TokenType, const char*);
static bool match(TokenType);
static void error_at(Token*, const char*);
static void error_at_current(const char*);
static void error_at_previous(const char*);
static void sync();

bool compile(const char* source) {
  init_lexer();
  init_scan(source);
  perm_arena = arena_create(MiB(64));
  parser.hadError = false;
  parser.panicMode = false;
  parser.source = source;

  advance();

  while (!match(TOKEN_EOF)) {
    parser.panicMode = false;
    parse_statement();
    if (parser.panicMode) sync();
  }
  bool result = !parser.hadError;
  arena_destroy(perm_arena);
  perm_arena = NULL;
  return result;
}

static void advance() {
  parser.previous = parser.current;
  for (;;) {
    parser.current = scan_token();
    if (parser.current.type != TOKEN_ERROR) break;
    if (!parser.panicMode) {
      char lexer_msg[128];
      snprintf(lexer_msg, sizeof(lexer_msg), "Lexer: Invalid character."); 
      error_at_current(lexer_msg);
      //fprintf(stderr, "Error at line %d: %.*s\n", parser.current.line, parser.current.length, parser.current.start);
      //parser.hadError = true;
      //parser.panicMode = true;
    }
  }
}

static const char* token_type_to_string(TokenType type) {
  switch (type) {
    case TOKEN_LEFT_PAREN: return "'('";
    case TOKEN_RIGHT_PAREN: return "')'";
    case TOKEN_LEFT_BRACE: return "'{'";
    case TOKEN_RIGHT_BRACE: return "'}'";
    case TOKEN_SEMICOLON: return "';'";
    case TOKEN_EOF: return "end of file";
    default: return "token";
  }
}

static void error_at(Token* token, const char* message) {
  if (parser.panicMode) return;
  parser.panicMode = true;
  parser.hadError = true;
  fprintf(stderr, "Error at line %d", token->line);
  if (token->type == TOKEN_EOF) {
    fprintf(stderr, " at end of file");
  } else {
    fprintf(stderr, " at '%.*s'", token->length, token->start);
  }
  fprintf(stderr, ": %s\n", message);

  if (token->type != TOKEN_EOF) {
    const char* line_start = token->start;
    while (line_start > parser.source && *(line_start - 1) != '\n') {
      line_start--;
    }

    const char* line_end = token->start;
    while (*line_end != '\n' && *line_end != '\0') {
      line_end++;
    }

    int line_length = (int)(line_end - line_start);
    fprintf(stderr, "    %.*s\n", line_length, line_start);

    fprintf(stderr, "    "); 
    for (const char* p = line_start; p < token->start; p++) {
      if (*p == '\t') {
        fprintf(stderr, "\t");
      } else {
        fprintf(stderr, " ");
      }
    }

    fprintf(stderr, "^\n");
  }
}
static void error_at_current(const char* message) {
  error_at(&parser.current, message);
}

static void error_at_previous(const char* message) {
  error_at(&parser.previous, message);
}

static void consume(TokenType type, const char* message) {
  if (parser.current.type == type) {
    advance();
    return;
  }
  char dynamic_message[256];
  if (parser.current.type == TOKEN_EOF) {
      snprintf(dynamic_message, sizeof(dynamic_message), "%s (Found end of file)", message);
  } else {
      snprintf(dynamic_message, sizeof(dynamic_message), "%s (Found '%.*s')", message, parser.current.length, parser.current.start);
  }
  error_at_current(dynamic_message);
}

static bool match(TokenType type) {
  if (parser.current.type != type) return false;
  advance();
  return true;
}

static void sync() {
  parser.panicMode = false;
  while (parser.current.type != TOKEN_EOF) {
    if (parser.previous.type == TOKEN_SEMICOLON) return;
    switch (parser.current.type) {
      case TOKEN_IF:
      case TOKEN_WHILE:
      case TOKEN_FOR:
      case TOKEN_RETURN:
        return;
      default:
        ;
    }
    advance();
  }
}

// parse functions

static AstNode* parse_statement() {
  if (match(TOKEN_IF)) return parse_if();
  if (match(TOKEN_LEFT_BRACE)) return parse_block();
  if (match(TOKEN_WHILE)) return parse_while();

  return parse_expression_stmt();
}

static AstNode* parse_expression_stmt() {
  AstNode *expr = parse_expression();
  consume(TOKEN_SEMICOLON, "Parser: Expected ';' at the end of instruction.");
  i32 line = (expr != NULL) ? expr->line : parser.previous.line;
  AstNode* stmt = make_node(perm_arena, NODE_STATEMENT, line);
  stmt->as.statement.expression = expr;
  return stmt;
}

static AstNode* parse_block() {
  AstNode *n = make_node(perm_arena, NODE_BLOCK, parser.previous.line);
  // i32 capacity = 8;
  // n->as.block.statements = malloc(sizeof(AstNode*) * capacity);
  n->as.block.statements = PUSH_ARRAY(perm_arena, AstNode*, 64);
  n->as.block.count = 0;

  while (parser.current.type != TOKEN_RIGHT_BRACE && parser.current.type != TOKEN_EOF) {
    AstNode* stmt = parse_statement();
    if (parser.panicMode) {
      sync();
      parser.panicMode = false;
    }
    if (stmt != NULL) {
      // if (n->as.block.count == capacity) {
      //   capacity *= 2;
      //   n->as.block.statements = realloc(n->as.block.statements, sizeof(AstNode*) * capacity);
      // }
      n->as.block.statements[n->as.block.count++] = stmt;
    }
  }

  consume(TOKEN_RIGHT_BRACE, "Parser: Expected '}' at the end of the block.");
  return n;
}

static AstNode* parse_if() {
  i32 line = parser.previous.line;
  consume(TOKEN_LEFT_PAREN, "Parser: Expected '(' after 'if'.");
  AstNode *condition = parse_expression();
  consume(TOKEN_RIGHT_PAREN, "Parser: Expected ')' after expression.");
  AstNode *then_branch = parse_statement();
  AstNode* else_branch = NULL;
  if (match(TOKEN_ELSE)) else_branch = parse_statement();
  AstNode *n = make_node(perm_arena, NODE_IF, line);
  n->as.if_stmt.condition = condition;
  n->as.if_stmt.thenBranch = then_branch;
  n->as.if_stmt.elseBranch = else_branch;
  return n;
}

static AstNode* parse_while() {
  i32 line = parser.previous.line;
  consume(TOKEN_LEFT_PAREN, "Parser: Expected '(' after 'while'.");
  AstNode *condition = parse_expression();
  consume(TOKEN_RIGHT_PAREN, "Parser: Expected ')' after expression.");
  AstNode *body = parse_statement();
  AstNode *n = make_node(perm_arena, NODE_WHILE, line);
  n->as.while_stmt.condition = condition;
  n->as.while_stmt.body = body;
  return n;
}

// pratt functions

static AstNode* parse_expression() {
  return parse_presedence(PREC_ASSIGN);
}

static AstNode* parse_presedence(Presedence p) {
  advance();
  PrefixFn prefix = get_rule(parser.previous.type)->prefix;
  if (!prefix) {
    error_at_previous("Parser: expected expression");
    return NULL;
  }

  AstNode* left = prefix();

  if (parser.panicMode) return left;

  while (p <= get_rule(parser.current.type)->p) {
    advance();
    InfixFn infix = get_rule(parser.previous.type)->infix;
    left = infix(left);

    if (parser.panicMode) return left;
  }
  return left;
}

static AstNode* number() {
  AstNode* n = make_node(perm_arena, NODE_LITERAL, parser.previous.line);
  if (parser.previous.type == TOKEN_INTEGER_LITERAL) {
    i32 val = 0;
    for (i32 i = 0; i < parser.previous.length; i++) {
      char c = parser.previous.start[i];
      val = (val * 10) + (c - '0');
    }
    n->as.literal.ival = val;
    n->as.literal.literalType = TOKEN_INTEGER_LITERAL;
  } else if (parser.previous.type == TOKEN_DOUBLE_LITERAL) {
    f64 val = 0.0;
    i32 i = 0;
    while (i < parser.previous.length && parser.previous.start[i] != '.') {
      char c = parser.previous.start[i];
      val = (val * 10.0) + (c - '0');
      i++;
    }
    if (i < parser.previous.length && parser.previous.start[i] == '.') {
      i++;
      f64 divisor = 10.0;
      while (i < parser.previous.length) {
        char c = parser.previous.start[i];
        val = val + ((c - '0') / divisor);
        divisor *= 10.0;
        i++;
      }
    }
    n->as.literal.dval = val;
    n->as.literal.literalType = TOKEN_DOUBLE_LITERAL;
  }

  return n;
}

static AstNode* variable() {
  AstNode* n = make_node(perm_arena, NODE_VARIABLE, parser.previous.line);
  i32 len = parser.previous.length;

  char *name = PUSH_ARRAY(perm_arena, char, len + 1);
  memcpy(name, parser.previous.start, len);
  name[len] = '\0';
  n->as.variable.name = name;
  
  // n->as.variable.name = malloc(len + 1);
  // for (i32 i = 0; i < len; i++)
  //   n->as.variable.name[i] = parser.previous.start[i];
  // n->as.variable.name[len] = '\0';

  return n;
}

static AstNode* grouping() {
  AstNode *expr = parse_expression();
  consume(TOKEN_RIGHT_PAREN, "Parser: Expected ')' after expression.\n");
  return expr;
}

static AstNode* unary() {
  TokenType op = parser.previous.type;
  i32 line = parser.previous.line;
  AstNode* right = parse_presedence(PREC_UNARY);
  AstNode* n = make_node(perm_arena, NODE_UNARY, line);
  n->as.unary.op = op;
  n->as.unary.right = right;
  return n;
}

static AstNode* binary(AstNode* left) {
  i32 line = parser.previous.line;
  TokenType op = parser.previous.type;
  ParseRule* rule = get_rule(op);

  AstNode* right = parse_presedence((Presedence)(rule->p + 1));

  if (parser.panicMode || right == NULL) return NULL;

  AstNode *n = make_node(perm_arena, NODE_BINARY, line);
  n->as.binary.left = left;
  n->as.binary.op = op;
  n->as.binary.right = right;
  return n;
}

static AstNode* assign(AstNode* target) {
  i32 line = parser.previous.line;
  TokenType op = parser.previous.type;
  if (target->type != NODE_VARIABLE) {
    error_at_previous("Parser: Objective of assignment invalid.");
    return NULL;
  }

  AstNode* val = parse_expression();
  if (parser.panicMode || val == NULL) return NULL;

  AstNode *n = make_node(perm_arena, NODE_ASSIGN, line);
  n->as.assign.target = target->as.variable.name;
  n->as.assign.op = op;
  n->as.assign.value = val;
  return n;
}
