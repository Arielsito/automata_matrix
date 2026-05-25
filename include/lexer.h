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
  TOKEN_VOID, TOKEN_VOLATILE, TOKEN_WHILE, TOKEN_INCLUDE,
  // Numbers
  TOKEN_INTEGER_LITERAL,
  TOKEN_DOUBLE_LITERAL,
  // Strings and chars
  TOKEN_STRING_LITERAL,
  TOKEN_CHAR_LITERAL,
  // Symbols
  TOKEN_LEFT_PAREN, TOKEN_RIGHT_PAREN, TOKEN_LEFT_BRACE, TOKEN_RIGHT_BRACE, TOKEN_LEFT_BRACKET, TOKEN_RIGHT_BRACKET,
  TOKEN_COMMA, TOKEN_DOT,
  TOKEN_SEMICOLON, TOKEN_COLON, TOKEN_QUESTION,
  TOKEN_MEMBER, TOKEN_TAG,
  // Arithmetic Operators
  TOKEN_MINUS, TOKEN_PLUS, TOKEN_STAR, TOKEN_SLASH, TOKEN_MOD,
  TOKEN_MINUS_MINUS, TOKEN_PLUS_PLUS,
  // Assignment Operators
  TOKEN_EQUAL,
  TOKEN_MINUS_EQUAL,TOKEN_PLUS_EQUAL,
  TOKEN_STAR_EQUAL, TOKEN_SLASH_EQUAL, TOKEN_MOD_EQUAL,
  TOKEN_LSE, TOKEN_RSE, TOKEN_AND_BITW_EQUAL, TOKEN_OR_BITW_EQUAL, TOKEN_XOR_BITW_EQUAL,
  // Relational Operators
  TOKEN_NOT_EQUAL, TOKEN_EQUAL_EQUAL, TOKEN_LESS, TOKEN_LESS_EQUAL,
  TOKEN_GREATER, TOKEN_GREATER_EQUAL, 
  // Logical Operators
  TOKEN_NOT, TOKEN_AND, TOKEN_OR,
  // BitWise Operators
  TOKEN_AND_BITW, TOKEN_OR_BITW, TOKEN_NOT_BITW, TOKEN_XOR_BITW, TOKEN_LEFT_SHIFT, TOKEN_RIGHT_SHIFT,

  TOKEN_LIB,
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
  ACTION_ANY
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
