#include "../include/common.h"
#include "../include/lexer.h"
#include "../include/matrices.h"

#define HASH_SIZE 64

typedef struct scanner {
  const char *start;
  const char *current;
  int line;
} Scanner;

Scanner scanner;

static Token make_token(TokenType);
static Token run_dfa(const Dfa*, i32);
static void skip_whitespace();
static u64 hash(const char*, i32);
static void add_keyword(const char*, TokenType);
static TokenType check_keyword(const char*, i32);

typedef struct hash_entry {
  const char *lexeme;
  TokenType type;
} HashEntry;

static HashEntry keywords[HASH_SIZE];

static u64 hash(const char *str, i32 len) {
  u64 hash = 5381;
  for (i32 i = 0; i < len; i++) {
    hash = ((hash << 5) + hash) + (u8)str[i];
  }
  return hash;
}

static void add_keyword(const char *name, TokenType type) {
  i32 len = strlen(name);
  u64 h = hash(name, len) % HASH_SIZE;
  while (keywords[h].lexeme != NULL) {
    h = (h + 1) % HASH_SIZE;
  }
  keywords[h].lexeme = name;
  keywords[h].type = type;
}

void init_lexer() {
  memset(keywords, 0, sizeof(keywords));
  #define KEYWORD(text, token) add_keyword(text, token);
  #include "keywords.def"
  #undef KEYWORD
}

void init_scan(const char *source) {
  scanner.start = source;
  scanner.current = source;
  scanner.line = 1;
}

Token scan_token() {
  skip_whitespace();
  scanner.start = scanner.current;

  if (*scanner.current == '\0') return make_token(TOKEN_EOF);

  const char *start_pos = scanner.current;
  Token t;

  t = run_dfa(&identifier, 0);
  if (t.type != TOKEN_ERROR) {
    t.type = check_keyword(t.start, t.length);
    return t;
  }
  
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
  token.line = scanner.line;
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

static TokenType check_keyword(const char *start, i32 len) {
  u64 h = hash(start, len) % HASH_SIZE;
  
  while(keywords[h].lexeme != NULL) {
    if ((i32)strlen(keywords[h].lexeme) == len &&
        strncmp(start, keywords[h].lexeme, len) == 0) {
      return keywords[h].type;
    } 
    h = (h + 1) % HASH_SIZE;
  }
  return TOKEN_IDENTIFIER;
}

static void skip_whitespace() {
  for (;;) {
    char c = *scanner.current;
    switch(c) {
      case ' ':
      case '\r':
      case '\t':
        scanner.current++;
        break;
      case '\n':
        scanner.line++;
        scanner.current++;
        break;
      case '/':
        if (scanner.current[1] == '/') {
          while (*scanner.current != '\n' && *scanner.current != '\0') scanner.current++;
        } else if (scanner.current[1] == '*') {
          scanner.current += 2;
          while (1) {
            if (*scanner.current == '\0') return;
            if (*scanner.current == '\n') scanner.line++;
            if (*scanner.current == '*' && scanner.current[1] == '/') {
              scanner.current += 2;
               break;
            }
            scanner.current++;
          }
        } else return;
        break;
      default:
        return;
    }
  }
}
