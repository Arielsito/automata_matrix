#include <criterion/criterion.h>
#include <criterion/parameterized.h>
#include "../include/lexer.h"

// initialize scanner
static void initialize_lexer(void) {
  init_lexer();
}
TestSuite(lexer_kws, .init = initialize_lexer);
static Token next_token(const char *src) {
  init_scan(src);
  return scan_token();
}
// keywords
typedef struct {
  char input[16];
  TokenType expected;
} KeywordCase;

ParameterizedTestParameters(lexer_kws, keywords) {
  static const KeywordCase cases[] = {
    { "auto", TOKEN_AUTO },
    { "break", TOKEN_BREAK },
    { "case", TOKEN_CASE },
    { "char", TOKEN_CHAR },
    { "const", TOKEN_CONST },
    { "continue", TOKEN_CONTINUE },
    { "default", TOKEN_DEFAULT },
    { "do", TOKEN_DO },
    { "double", TOKEN_DOUBLE },
    { "else", TOKEN_ELSE },
    { "enum", TOKEN_ENUM },
    { "extern", TOKEN_EXTERN },
    { "float", TOKEN_FLOAT },
    { "for", TOKEN_FOR },
    { "goto", TOKEN_GOTO },
    { "if", TOKEN_IF },
    { "int", TOKEN_INT },
    { "long", TOKEN_LONG },
    { "register", TOKEN_REGISTER },
    { "return", TOKEN_RETURN },
    { "short", TOKEN_SHORT },
    { "signed", TOKEN_SIGNED },
    { "sizeof", TOKEN_SIZEOF },
    { "static", TOKEN_STATIC },
    { "struct", TOKEN_STRUCT },
    { "switch", TOKEN_SWITCH },
    { "typedef", TOKEN_TYPEDEF },
    { "union", TOKEN_UNION },
    { "unsigned", TOKEN_UNSIGNED },
    { "void", TOKEN_VOID },
    { "volatile", TOKEN_VOLATILE },
    { "while", TOKEN_WHILE },
    { "include", TOKEN_INCLUDE },
  };
  size_t count = sizeof(cases) / sizeof(KeywordCase);
  return cr_make_param_array(KeywordCase, cases, count);
}
ParameterizedTest(KeywordCase *tc, lexer_kws, keywords) {
  Token t = next_token(tc->input);
  cr_assert_eq(t.type, tc->expected,
      "Error in keyword '%s': expected '%d' but got '%d'", tc->input, tc->expected, t.type
      );
}
