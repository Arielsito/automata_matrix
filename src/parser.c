#include "../include/common.h"
#include "../include/lexer.h"
#include "../include/parser.h"

typedef struct parser {
  Token current;
  Token previous;
  bool hadError;
} Parser;

static Parser parser;

static void advance();
static void consume(TokenType, const char*);

bool compile(const char* source) {
  init_lexer();
  init_scan(source);
  parser.hadError = false;
  advance();
  consume(TOKEN_EOF, "Unexpected end of input.");
  return !parser.hadError;
}

static void advance() {
  parser.previous = parser.current;
  for (;;) {
    parser.current = scan_token();
    if (parser.current.type != TOKEN_ERROR) break;
    fprintf(stderr, "Error at line %d: %.*s\n", parser.current.line, parser.current.length, parser.current.start);
    parser.hadError = true;
  }
}

static void consume(TokenType type, const char* message) {
  if (parser.current.type == type) {
    advance();
    return;
  }
  fprintf(stderr, "Error at line %d: %s\n", parser.current.line, message);
  parser.hadError = true;
}
