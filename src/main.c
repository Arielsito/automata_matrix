#include "../include/lexer.h"

static void eval_expression() {
  char buff[1024];
  for (;;) {
    printf("> ");
    if (!fgets(buff, sizeof(buff), stdin)) {
      printf("\n");
      break;
    }
    init_scan(buff);
    for (;;) {
      Token t = scan_token();
      printf("[%-12s] | Length: %d | Lexeme: '%.*s'\n", token_type_name(t.type), t.length, t.length, t.start);
      if (t.type == TOKEN_EOF) break;
    }
  }
}

int main(i32 argc, char *argv[]) {
  if (argc == 1) eval_expression();
  else if (argc == 2) printf("Pendiente");
  else {
    fprintf(stderr, "Error: Too many arguments.\n");
    exit(64);
  }
  return 0;
}
