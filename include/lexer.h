#ifndef LEXER_H
#define LEXER_H

#include "../include/common.h"

typedef enum token_type {
  // Characters
  TOKEN_LETTER,
  TOKEN_DIGIT,
  // Identifiers
  TOKEN_IDENTIFIER,
  TOKEN_AUTO, TOKEN_BREAK, TOKEN_CASE, TOKEN_CHAR, TOKEN_CONST, TOKEN_CONTINUE, TOKEN_DEFAULT,
  TOKEN_DO, TOKEN_DOUBLE, TOKEN_ELSE, TOKEN_ENUM, TOKEN_EXTERN, TOKEN_FLOAT, TOKEN_FOR, TOKEN_GOTO,
  TOKEN_IF, TOKEN_INT, TOKEN_LONG, TOKEN_REGISTER, TOKEN_RETURN, TOKEN_SHORT, TOKEN_SIGNED,
  TOKEN_SIZEOF, TOKEN_STATIC, TOKEN_STRUCT, TOKEN_SWITCH, TOKEN_TYPEDEF, TOKEN_UNION, TOKEN_UNSIGNED,
  TOKEN_VOID, TOKEN_VOLATILE, TOKEN_WHILE,
  // Numbers
  TOKEN_INTEGER_LITERAL,
  TOKEN_DOUBLE_LITERAL,
  TOKEN_EOF,
  TOKEN_ERROR
} TokenType;

typedef struct token {
  TokenType type;
  const char *start;
  i32 length;
  i32 line;
} Token;

typedef enum action {
  ACTION_NONE,
  ACTION_CALL,
  ACTION_ACCEPT,
  ACTION_REJECT,
} Action;

typedef struct dfa Dfa;

typedef struct state {
  Action action;
  union {
    char c;
    const struct dfa *sub;
    TokenType token_type;
  } payload;
  i32 next_state;
} State;

struct dfa {
  const State *table;
  i32 states;
  i32 cols;
};

typedef struct scanner Scanner;

void init_lexer();
void init_scan(const char*);
Token scan_token();
const char* token_type_name(TokenType);

#endif // LEXER_H
