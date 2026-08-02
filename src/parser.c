#include "../include/common.h"
#include "../include/lexer.h"
#include <stdint.h>
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

static AstNode** grow_array(Arena *arena, AstNode **arr, i32 *cap) {
  *cap *= 2;
  if (*cap > 1 << 20) return NULL;
  AstNode **new = PUSH_ARRAY(arena, AstNode*, *cap);
  memcpy(new, arr, (*cap / 2) * sizeof(AstNode*));
  return new;
}

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
static AstNode* parse_return();
static AstNode* parse_break();
static AstNode* parse_continue();
static AstNode* parse_if();
static AstNode* parse_switch();
static AstNode* parse_case();
static AstNode* parse_while();
static AstNode* parse_do_while();
static AstNode* parse_for();
static AstNode* parse_expression_stmt();

// pratt parser for math expressions
static AstNode* parse_expression();
static AstNode* parse_presedence(Presedence);
static AstNode* number();
static AstNode* l_string();
static AstNode* l_char();
static AstNode* variable();
static AstNode* grouping();
static AstNode* unary();
static AstNode* binary(AstNode*);
static AstNode* assign(AstNode*);
static AstNode* postfix(AstNode*);

static ParseRule rules[] = {
  [TOKEN_INTEGER_LITERAL] = { number, NULL, PREC_NONE },
  [TOKEN_DOUBLE_LITERAL] = { number, NULL, PREC_NONE },
  [TOKEN_IDENTIFIER] = { variable, NULL, PREC_NONE },
  [TOKEN_LEFT_PAREN] = { grouping, NULL, PREC_NONE },

  [TOKEN_NOT] = { unary, NULL, PREC_NONE },
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

  [TOKEN_PLUS_PLUS] = { unary, postfix, PREC_UNARY },
  [TOKEN_MINUS_MINUS] = { unary, postfix, PREC_UNARY },

  [TOKEN_EQUAL_EQUAL] = { NULL, binary, PREC_EQUAL },
  [TOKEN_NOT_EQUAL] = { NULL, binary, PREC_EQUAL },
  [TOKEN_LESS] = { NULL, binary, PREC_COMPARE },
  [TOKEN_LESS_EQUAL] = { NULL, binary, PREC_COMPARE },
  [TOKEN_GREATER] = { NULL, binary, PREC_COMPARE },
  [TOKEN_GREATER_EQUAL] = { NULL, binary, PREC_COMPARE },

  [TOKEN_AND] = { NULL, binary, PREC_AND },
  [TOKEN_OR] = { NULL, binary, PREC_OR },

  [TOKEN_LEFT_SHIFT] = { NULL, binary, PREC_SHIFT },
  [TOKEN_RIGHT_SHIFT] = { NULL, binary, PREC_SHIFT },
  [TOKEN_LSE] = { NULL, assign, PREC_ASSIGN },
  [TOKEN_RSE] = { NULL, assign, PREC_ASSIGN },

  [TOKEN_NOT_BITW] = { unary, NULL, PREC_NONE },
  [TOKEN_AND_BITW] = { NULL, binary, PREC_BITW },
  [TOKEN_OR_BITW] = { NULL, binary, PREC_BITW },
  [TOKEN_XOR_BITW] = { NULL, binary, PREC_BITW },
  [TOKEN_AND_BITW_EQUAL] = { NULL, assign, PREC_ASSIGN },
  [TOKEN_OR_BITW_EQUAL] = { NULL, assign, PREC_ASSIGN },
  [TOKEN_XOR_BITW_EQUAL] = { NULL, assign, PREC_ASSIGN },

  [TOKEN_STRING_LITERAL] = { l_string, NULL, PREC_NONE },
  [TOKEN_CHAR_LITERAL] = { l_char, NULL, PREC_NONE },

  [TOKEN_EOF] = { NULL, NULL, PREC_NONE },
  [TOKEN_ERROR] = { NULL, NULL, PREC_NONE },
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
static Arena *perm_arena; 

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
  if (perm_arena == NULL) {
    fprintf(stderr, "Error: Out of memory.");
    return false;
  }
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
      const char *msg = take_lexer_error();
      error_at_current(msg != NULL ? msg : "Lexer: Invalid character.");
      //fprintf(stderr, "Error at line %d: %.*s\n", parser.current.line, parser.current.length, parser.current.start);
      //parser.hadError = true;
      //parser.panicMode = true;
    }
  }
}

// static const char* token_type_to_string(TokenType type) {
//   switch (type) {
//     case TOKEN_LEFT_PAREN: return "'('";
//     case TOKEN_RIGHT_PAREN: return "')'";
//     case TOKEN_LEFT_BRACE: return "'{'";
//     case TOKEN_RIGHT_BRACE: return "'}'";
//     case TOKEN_SEMICOLON: return "';'";
//     case TOKEN_EOF: return "end of file";
//     default: return "token";
//   }
// }

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
  // NULL STATEMENTS
  if (match(TOKEN_SEMICOLON))
    return make_node(perm_arena, NODE_STATEMENT, parser.previous.line);

  if (match(TOKEN_RETURN)) return parse_return();
  if (match(TOKEN_BREAK)) return parse_break();
  if (match(TOKEN_CONTINUE)) return parse_continue();
  if (match(TOKEN_IF)) return parse_if();
  if (match(TOKEN_SWITCH)) return parse_switch();
  if (match(TOKEN_LEFT_BRACE)) return parse_block();
  if (match(TOKEN_WHILE)) return parse_while();
  if (match(TOKEN_DO)) return parse_do_while();
  if (match(TOKEN_FOR)) return parse_for();

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
  n->as.block.capacity = 64;
  n->as.block.statements = PUSH_ARRAY(perm_arena, AstNode*, 64);
  n->as.block.count = 0;

  while (parser.current.type != TOKEN_RIGHT_BRACE && parser.current.type != TOKEN_EOF) {
    AstNode* stmt = parse_statement();
    if (parser.panicMode) {
      sync();
      parser.panicMode = false;
    }
    if (stmt != NULL) {
      if (n->as.block.count == n->as.block.capacity) {
        AstNode **n_arr = grow_array(perm_arena, n->as.block.statements, &n->as.block.capacity);
        if (n_arr == NULL) {
          error_at_current("Parser: too many statements.");
          return NULL;
        }
        n->as.block.statements = n_arr;
      }
      n->as.block.statements[n->as.block.count++] = stmt;
    }
  }

  consume(TOKEN_RIGHT_BRACE, "Parser: Expected '}' at the end of the block.");
  return n;
}

static AstNode* parse_return() {
  i32 line = parser.previous.line;
  AstNode *n = make_node(perm_arena, NODE_RETURN, line);
  if (parser.current.type == TOKEN_SEMICOLON) // return ;
    n->as.return_stmt.value = NULL;
  else
    n->as.return_stmt.value = parse_expression();

  consume(TOKEN_SEMICOLON, "Parser: Expected ';' at the end of instruction.");
  return n;
}

static AstNode* parse_break() {
  i32 line = parser.previous.line;
  consume(TOKEN_SEMICOLON, "Parser: Expected ';' at the end of instruction.");
  return make_node(perm_arena, NODE_BREAK, line);
}

static AstNode* parse_continue() {
  i32 line = parser.previous.line;
  consume(TOKEN_SEMICOLON, "Parser: Expected ';' at the end of instruction.");
  return make_node(perm_arena, NODE_CONTINUE, line);
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

static AstNode* parse_switch() {
  i32 line = parser.previous.line;
  consume(TOKEN_LEFT_PAREN, "Parser: Expected '(' after 'switch'.");
  AstNode *condition = parse_expression();
  consume(TOKEN_RIGHT_PAREN, "Parser: Expected ')' after expression.");

  i32 cap = 64;
  AstNode **cases = PUSH_ARRAY(perm_arena, AstNode*, cap);
  i32 count = 0;

  if (match(TOKEN_LEFT_BRACE)) {
    while (
        parser.current.type != TOKEN_RIGHT_BRACE &&
        parser.current.type != TOKEN_EOF
    ) {
      if (count == cap) {
        AstNode **n_arr = grow_array(perm_arena, cases, &cap);
        if (n_arr == NULL) {
          error_at_current("Parser: too many cases.");
          return NULL;
        }
        cases = n_arr;
      }
      cases[count++] = match(TOKEN_CASE) || match(TOKEN_DEFAULT) 
        ? parse_case() : parse_statement();
    }

    consume(TOKEN_RIGHT_BRACE, "Parser: Expected '}' after switch.");
  } else
    consume(TOKEN_SEMICOLON, "Parser: Expected ';' or '{' after switch.");


  AstNode *n = make_node(perm_arena, NODE_SWITCH, line);
  n->as.switch_stmt.condition = condition;
  n->as.switch_stmt.cases = cases;
  n->as.switch_stmt.count = count;

  return n;
}

static AstNode* parse_case() {
  i32 line = parser.previous.line;
  bool is_default = parser.previous.type == TOKEN_DEFAULT;

  AstNode *expression = NULL;
  if (!is_default)
    expression = parse_expression();
  consume(TOKEN_COLON, "Parser: Expected ':' after case.");

  i32 cap = 64;
  AstNode **statements = PUSH_ARRAY(perm_arena, AstNode*, cap);
  i32 count = 0;

  while (
      parser.current.type != TOKEN_CASE         &&
      parser.current.type != TOKEN_DEFAULT      &&
      parser.current.type != TOKEN_RIGHT_BRACE  &&
      parser.current.type != TOKEN_EOF
  ) {
    if (count == cap) {
      AstNode **n_arr = grow_array(perm_arena, statements, &cap);
      if (n_arr == NULL) {
        error_at_current("Parser: too many statements.");
        return NULL;
      }
      statements = n_arr;
    }
    statements[count++] = parse_statement();
  }

  AstNode *n = make_node(perm_arena, NODE_CASE, line);
  n->as.case_stmt.expression = expression;
  n->as.case_stmt.statements = statements;
  n->as.case_stmt.count = count;
  n->as.case_stmt.is_default = is_default;

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

static AstNode* parse_do_while() {
  i32 line = parser.previous.line;
  AstNode *body = parse_statement();
  consume(TOKEN_WHILE, "Parser: Expected 'while' after statement.");
  consume(TOKEN_LEFT_PAREN, "Parser: Expected '(' after 'while'.");
  AstNode *condition = parse_expression();
  consume(TOKEN_RIGHT_PAREN, "Parser: Expected ')' after expression.");
  consume(TOKEN_SEMICOLON, "Parser: Expected ';' at the end of instruction.");
  AstNode *n = make_node(perm_arena, NODE_DO_WHILE, line);
  n->as.while_stmt.body = body;
  n->as.while_stmt.condition = condition;
  return n;
}

static AstNode* parse_for() {
  i32 line = parser.previous.line;
  consume(TOKEN_LEFT_PAREN, "Parser: Expected '(' after 'for'.");

  // for(;;) -> expressions are optional
  AstNode *init = NULL;
  if (parser.current.type != TOKEN_SEMICOLON)
    init = parse_expression_stmt();
  else
    advance();

  AstNode *condition = NULL;
  if (parser.current.type != TOKEN_SEMICOLON)
    condition = parse_expression();
  consume(TOKEN_SEMICOLON, "Parser: Expected ';' after for condition.");


  AstNode *update = NULL;
  if (parser.current.type != TOKEN_RIGHT_PAREN)
    update = parse_expression();

  consume(TOKEN_RIGHT_PAREN, "Parser: Expected ')' after expression.");
  AstNode *body = parse_statement();
  AstNode *n = make_node(perm_arena, NODE_FOR, line);
  n->as.for_stmt.init = init;
  n->as.for_stmt.condition = condition;
  n->as.for_stmt.update = update;
  n->as.for_stmt.body = body;
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
    error_at_previous("Parser: expected expression.");
    return NULL;
  }

  AstNode* left = prefix();

  if (parser.panicMode) return left;

  while (p <= get_rule(parser.current.type)->p) {
    advance();
    InfixFn infix = get_rule(parser.previous.type)->infix;
    if (!infix) {
      error_at_previous("Parser: unexpected token in expression.");
      return left;
    }

    left = infix(left);
    if (parser.panicMode) return left;
  }
  return left;
}

static AstNode* number() {
  AstNode* n = make_node(perm_arena, NODE_LITERAL, parser.previous.line);
  if (parser.previous.type == TOKEN_INTEGER_LITERAL) {
    i64 val = 0;
    for (i32 i = 0; i < parser.previous.length; i++) {
      char c = parser.previous.start[i];
      val = (val * 10) + (c - '0');
      if (val > INT32_MAX) {
        error_at_previous("Parser: Integer literal out of range.");
        return NULL;
      }
    }
    n->as.literal.ival = (i32)val;
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

static AstNode* l_string() {
  AstNode* n = make_node(perm_arena, NODE_LITERAL, parser.previous.line);
  const char *src = parser.previous.start;
  i32 len = parser.previous.length;

  if (len < 2 || src[0] != '"' || src[len - 1] != '"') {
    error_at_current("Parser: Malformed string literal.");
    return NULL;
  }

  char *out = PUSH_ARRAY(perm_arena, char, len);
  i32 out_len = 0;
  for (i32 i = 1; i < len - 1; i++) {
    char c = src[i];
    if (c == '\\') {
      if (i + 1 >= len - 1) {
        error_at_previous("Parser: Unterminated escape sequence in string literal.");
        return NULL;
      }
      switch (src[++i]) {
        case 'n': c = '\n'; break;
        case 't': c = '\t'; break;
        case 'r': c = '\r'; break;
        case '0': c = '\0'; break;
        case '\\': c = '\\'; break;
        case '"': c = '"'; break;
        case '\'': c = '\''; break;
        default:
          error_at_previous("Parser: Unknown escape sequence in string literal.");
          return NULL;
      }
    }
    out[out_len++] = c;
  }
  out[out_len] = '\0';

  n->as.literal.sval = out;
  n->as.literal.literalType = TOKEN_STRING_LITERAL;

  return n;
}

static AstNode* l_char() {
  AstNode* n = make_node(perm_arena, NODE_LITERAL, parser.previous.line);
  const char *src = parser.previous.start;
  i32 len = parser.previous.length;

  if (len < 3 || src[0] != '\'' || src[len - 1] != '\'') {
    error_at_current("Parser: Malformed char literal.");
    return NULL;
  }

  char c;
  if (len == 3)
    c = src[1];
  else if (len == 4 && src[1] == '\\') {
    switch (src[2]) {
      case 'n': c = '\n'; break;
      case 't': c = '\t'; break;
      case 'r': c = '\r'; break;
      case '0': c = '\0'; break;
      case '\\': c = '\\'; break;
      case '"': c = '"'; break;
      case '\'': c = '\''; break;
      default:
        error_at_current("Parser: Unknown escape sequence in char literal.");
        return NULL;
    }
  } else {
    error_at_previous("Parser: Char literal must contain exactly one character.");
    return NULL;
  }

  n->as.literal.cval = c;
  n->as.literal.literalType = TOKEN_CHAR_LITERAL;

  return n;
}

static AstNode* variable() {
  AstNode* n = make_node(perm_arena, NODE_VARIABLE, parser.previous.line);
  i32 len = parser.previous.length;

  char *name = PUSH_ARRAY(perm_arena, char, len + 1);
  memcpy(name, parser.previous.start, len);
  name[len] = '\0';
  n->as.variable.name = name;

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

  AstNode* right = parse_presedence((Presedence)((i32)rule->p + 1));

  if (parser.panicMode || right == NULL || left == NULL) return NULL;

  AstNode *n = make_node(perm_arena, NODE_BINARY, line);
  n->as.binary.left = left;
  n->as.binary.op = op;
  n->as.binary.right = right;
  return n;
}

static AstNode* assign(AstNode* target) {
  if (target == NULL) return NULL;
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

static AstNode* postfix(AstNode* left) {
  if (left == NULL) return NULL;
  i32 line = parser.previous.line;
  TokenType op = parser.previous.type;
  AstNode *n = make_node(perm_arena, NODE_UNARY, line);
  n->as.unary.op = op;
  n->as.unary.right = left;
  return n;
}
