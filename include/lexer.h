#ifndef LEXER_H
#define LEXER_H

#include "../include/common.h"

typedef enum token_type {
  TOKEN_IDENTIFIER,
  TOKEN_NUMBER,
  TOKEN_LETTER,
  TOKEN_EOF,
  TOKEN_ERROR
} TokenType;

typedef struct token {
  TokenType type;
  const char *start;
  i32 length;
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

void init_scan(const char*);
Token scan_token();
const char* token_type_name(TokenType);

#endif // LEXER_H
