#include <criterion/criterion.h>
#include <criterion/parameterized.h>
#include "../include/lexer.h"

// initialize scanner
static void initialize_lexer(void) {
  init_lexer();
}

TestSuite(lexer_boundary, .init = initialize_lexer);
static Token next_token(const char *src) {
  init_scan(src);
  return scan_token();
}

typedef struct {
  char input[32];
  TokenType expected;
} BoundaryCase;

ParameterizedTestParameters(lexer_boundary, keyword_limits) {
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
 
ParameterizedTest(BoundaryCase *tc, lexer_boundary, keyword_limits) {
  Token t = next_token(tc->input);
  cr_assert_eq(t.type, tc->expected,
      "Error in '%s': expected %d but got %d", tc->input, tc->expected, t.type
      );
  Token e = scan_token();
  cr_assert_eq(e.type, TOKEN_EOF,
      "'%s' must be consumed as a SINGLE token", tc->input
      );
}

Test(lexer_boundary, keyword_in_stream) {
  init_scan("in include");
  Token a = scan_token();
  cr_assert_eq(a.type, TOKEN_IDENTIFIER);
  Token b = scan_token();
  cr_assert_eq(b.type, TOKEN_INCLUDE);
  Token e = scan_token();
  cr_assert_eq(e.type, TOKEN_EOF);
}
