#include "../include/lexer.h"
#include "../include/debug.h"
#include <stdio.h>
#include <stdlib.h>

static void parse_tokens(const char *source) {
  init_lexer();
  init_scan(source);
  for (;;) {
    Token t = scan_token();
    printf("[%-15s] | Line: %d | Length: %d | Lexeme: '%.*s'\n", token_type_name(t.type), t.line, t.length, t.length, t.start);
    if (t.type == TOKEN_EOF) break;
  }
}

static void eval_expression() {
  char buff[1024];
  for (;;) {
    printf("> ");
    if (!fgets(buff, sizeof(buff), stdin)) {
      printf("\n");
      break;
    }
    parse_tokens(buff);
  }
}

static char *readFile(const char* path) {
  FILE *file = fopen(path, "rb");
  if (file == NULL) {
    fprintf(stderr, "Could not open file '%s'.\n", path);
    exit(74);
  }
  fseek(file, 0L, SEEK_END);
  size_t fileSize = ftell(file);
  rewind(file);
  char *buffer = (char*)malloc(fileSize + 1);
  if (buffer == NULL) {
    fprintf(stderr, "Not enough memory to read '%s'.\n", path);
    exit(74);
  }
  size_t bytesRead = fread(buffer, sizeof(char), fileSize, file);
  if (bytesRead < fileSize) {
    fprintf(stderr, "Could not read file '%s'.\n", path);
    exit(74);
  }
  buffer[bytesRead] = '\0';
  fclose(file);
  return buffer;
}

static void runFile(const char *path) {
  char *source = readFile(path);
  parse_tokens(source);
  free(source);
}

int main(i32 argc, char *argv[]) {
  if (argc == 1) eval_expression();
  else if (argc == 2) runFile(argv[1]);
  else {
    fprintf(stderr, "Error: Too many arguments.\n");
    exit(64);
  }
  return 0;
}
