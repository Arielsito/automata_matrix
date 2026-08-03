#include <criterion/criterion.h>
#include <criterion/internal/assert.h>
#include <criterion/internal/test.h>
#include <criterion/parameterized.h>
#include <string.h>
#include "lexer.h"

// initialize scanner
static void initialize_lexer(void) {
  init_lexer();
}
TestSuite(lexer_numbers, .init = initialize_lexer);
static Token next_token(const char *src) {
  init_scan(src);
  return scan_token();
}

typedef struct { char input[40]; } NumCase;

ParameterizedTestParameters(lexer_numbers, integer_literals) {
  static const NumCase cases[] = {
    { "0" }, { "1" }, { "9" }, { "123" }, { "999999999" },
    { "0000" }, { "12345678901234567890" },
  };
  return cr_make_param_array(NumCase, cases,
      sizeof(cases) / sizeof(NumCase));
}

ParameterizedTest(NumCase *tc, lexer_numbers, integer_literals) {
  Token t = next_token(tc->input);
  cr_assert_eq(t.type, TOKEN_INTEGER_LITERAL,
      "Error in '%s': expected INTEGER but got %d", tc->input, t.type
      );
  cr_assert_eq(t.length, (i32)strlen(tc->input),
      "'%s' complete (len %d, got %d)",
      tc->input, (i32)strlen(tc->input), t.length
      );
  Token e = scan_token();
  cr_assert_eq(e.type, TOKEN_EOF,
      "'%s' most be a SINGLE token", tc->input
      );
}

ParameterizedTestParameters(lexer_numbers, double_literals) {
  static const NumCase cases[] = {
    { "1.5" }, { "0.5" }, { ".5" }, { "123.456" }, { "123." },
    { "1." }, { "0.0" }, { "123456.789" },
  };
  return cr_make_param_array(NumCase, cases,
      sizeof(cases) / sizeof(NumCase));
}

ParameterizedTest(NumCase *tc, lexer_numbers, double_literals) {
  Token t = next_token(tc->input);
  cr_assert_eq(t.type, TOKEN_DOUBLE_LITERAL,
      "Error in '%s': expected DOUBLE but got %d", tc->input, t.type);
  cr_assert_eq(t.length, (i32)strlen(tc->input),
      "'%s' complete (len %d, got %d)",
      tc->input, (i32)strlen(tc->input), t.length);
  Token e = scan_token();
  cr_assert_eq(e.type, TOKEN_EOF,
      "'%s' most be a SINGLE token", tc->input);
}

typedef struct {
  char input[16];
  TokenType expected[5];
} NumStreamCase;

ParameterizedTestParameters(lexer_numbers, number_streams) {
  const static NumStreamCase cases[] = {
    { "1.2.3", { TOKEN_DOUBLE_LITERAL, TOKEN_DOUBLE_LITERAL, TOKEN_EOF } },
    { "12a", { TOKEN_INTEGER_LITERAL, TOKEN_IDENTIFIER, TOKEN_EOF } },
    { "123abc", { TOKEN_INTEGER_LITERAL, TOKEN_IDENTIFIER, TOKEN_EOF } },
    { "-5", { TOKEN_MINUS, TOKEN_INTEGER_LITERAL, TOKEN_EOF } },
    { "x1.5", { TOKEN_IDENTIFIER, TOKEN_DOUBLE_LITERAL, TOKEN_EOF } },
    { "1.5x", { TOKEN_DOUBLE_LITERAL, TOKEN_IDENTIFIER, TOKEN_EOF } },
    { "0x15F", { TOKEN_INTEGER_LITERAL, TOKEN_IDENTIFIER, TOKEN_EOF } },
    { "1e5", { TOKEN_INTEGER_LITERAL, TOKEN_IDENTIFIER, TOKEN_EOF } },
    { ".", { TOKEN_DOT, TOKEN_EOF } },
    { "..", { TOKEN_DOT, TOKEN_DOT, TOKEN_EOF } },
    { "5 + 3", { TOKEN_INTEGER_LITERAL, TOKEN_PLUS, TOKEN_INTEGER_LITERAL, TOKEN_EOF } },
    { "1.5+2", { TOKEN_DOUBLE_LITERAL, TOKEN_PLUS, TOKEN_INTEGER_LITERAL, TOKEN_EOF } },
  };
  return cr_make_param_array(NumStreamCase, cases, 
      sizeof(cases) / sizeof(NumStreamCase)
      );
}

ParameterizedTest(NumStreamCase *tc, lexer_numbers, number_streams) {
  init_scan(tc->input);
  for (i32 i = 0; tc->expected[i] != TOKEN_EOF; i++) {
    Token t = scan_token();
    cr_assert_eq(t.type, tc->expected[i],
        "Token %d of '%s': expected %d but got %d ('%.*s')",
        i, tc->input, tc->expected[i], t.type, t.length, t.start
        );
  }
  Token e = scan_token();
  cr_assert_eq(e.type, TOKEN_EOF,
      "'%s' must end in EOF", tc->input
      );
}

Test(lexer_numbers, number_at_eof) {
  Token t = next_token("123");
  cr_assert_eq(t.type, TOKEN_INTEGER_LITERAL);
  cr_assert_eq(t.length, 3, "len 3, got %d", t.length);
  Token d = next_token("1.5");
  cr_assert_eq(d.type, TOKEN_DOUBLE_LITERAL);
  cr_assert_eq(d.length, 3, "len 3, got %d", d.length);
}

Test(lexer_numbers, long_number_literal) {
  char buf[51];
  memset(buf, '7', 50);
  buf[50] = '\0';

  Token t = next_token(buf);
  cr_assert_eq(t.type, TOKEN_INTEGER_LITERAL);
  cr_assert_eq(t.length, 50, "len 50, got %d", t.length);
  Token e = scan_token();
  cr_assert_eq(e.type, TOKEN_EOF);
}
