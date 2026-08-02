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
  cr_assert_eq(t.length, (i32)strlen(tc->input),
      "'%s' completed (len %d, got %d)", tc->input, (i32)strlen(tc->input), t.length
      );
  Token e = scan_token();
  cr_assert_eq(e.type, TOKEN_EOF,
      "'%s' most be a SINGLE token", tc->input
      );
}

typedef struct {
  char input[32];
  TokenType expected;
} BoundaryCase;

ParameterizedTestParameters(lexer_kws, keyword_limits) {
  static const BoundaryCase cases[] = {
    { "ifx", TOKEN_IDENTIFIER },
    { "if2", TOKEN_IDENTIFIER },
    { "if_x", TOKEN_IDENTIFIER },
    { "dox", TOKEN_IDENTIFIER },
    { "dou", TOKEN_IDENTIFIER },
    { "in", TOKEN_IDENTIFIER },
    { "includex", TOKEN_IDENTIFIER },
    { "autoX", TOKEN_IDENTIFIER },
    { "break2", TOKEN_IDENTIFIER },
  };
  return cr_make_param_array(BoundaryCase, cases,
      sizeof(cases) / sizeof(BoundaryCase)
      );
}
 
ParameterizedTest(BoundaryCase *tc, lexer_kws, keyword_limits) {
  Token t = next_token(tc->input);
  cr_assert_eq(t.type, tc->expected,
      "Error in '%s': expected %d but got %d", tc->input, tc->expected, t.type
      );
  Token e = scan_token();
  cr_assert_eq(e.type, TOKEN_EOF,
      "'%s' must be consumed as a SINGLE token", tc->input
      );
}

Test(lexer_kws, keyword_in_stream) {
  init_scan("in include");
  Token a = scan_token();
  cr_assert_eq(a.type, TOKEN_IDENTIFIER);
  Token b = scan_token();
  cr_assert_eq(b.type, TOKEN_INCLUDE);
  Token e = scan_token();
  cr_assert_eq(e.type, TOKEN_EOF);
}
