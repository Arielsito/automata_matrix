#include "../include/lexer.h"
#include "../include/matrices.h"

const char* token_type_name(TokenType type) {
  switch (type) {
    case TOKEN_IDENTIFIER: return "IDENTIFIER";
    case TOKEN_NUMBER: return "NUMBER";
    case TOKEN_LETTER: return "LETTER";
    case TOKEN_ERROR: return "ERROR";
    case TOKEN_EOF: return "EOF";
    default: return "UNKNOWN";
  }
}

typedef struct scanner {
  const char *start;
  const char *current;
} Scanner;

Scanner scanner;

static Token make_token(TokenType);
static Token run_dfa(const Dfa*, i32);
static void skip_whitespace();

void init_scan(const char *source) {
  scanner.start = source;
  scanner.current = source;
}

Token scan_token() {
  skip_whitespace();
  scanner.start = scanner.current;

  if (*scanner.current == '\0') return make_token(TOKEN_EOF);

  const char *start_pos = scanner.current;
  Token t;

  t = run_dfa(&identifier, 0);
  if (t.type != TOKEN_ERROR) return t;
  
  scanner.current = start_pos;
  t = run_dfa(&number, 0);
  if (t.type != TOKEN_ERROR) return t;

  scanner.current = start_pos;
  scanner.current++;
  return make_token(TOKEN_ERROR);
}

static Token make_token(TokenType type) {
  Token token;
  token.type = type;
  token.start = scanner.start;
  token.length = (i32)(scanner.current - scanner.start);
  return token;
}

static Token run_dfa(const Dfa *dfa, i32 start) {
  i32 state = start;

  while (1) {
    char c = *scanner.current;
    for (i32 col = 0; col < dfa->cols; col++) {
      State s = CELL(dfa, state, col);

      switch(s.action) {
        case ACTION_NONE:
          if (s.payload.c == c) {
            scanner.current++;
            state = s.next_state;
            goto next;
          }
          break;

        case ACTION_CALL: {
          const char *saved = scanner.current;
          Token t = run_dfa((const Dfa *)s.payload.sub, 0);
          if (t.type != TOKEN_ERROR) {
            state = s.next_state;
            goto next;
          }
          scanner.current = saved;
          break;
        }

        case ACTION_ACCEPT:
          return make_token(s.payload.token_type);

        case ACTION_REJECT:
          return make_token(TOKEN_ERROR);
      }
    }
    return make_token(TOKEN_ERROR);
    next:;
  }
}

static void skip_whitespace() {
  for (;;) {
    char c = *scanner.current;
    switch(c) {
      case ' ':
      case '\r':
      case '\t':
      case '\n':
        scanner.current++;
        break;
      default:
        return;
    }
  }
}
