#include <criterion/criterion.h>
#include <criterion/parameterized.h>
#include <stddef.h>
#include <string.h>
#include "../include/lexer.h"

// initialize scanner
static void initialize_lexer(void) {
  init_lexer();
}
TestSuite(lexer_identifier, .init = initialize_lexer);
static Token next_token(const char *src) {
  init_scan(src);
  return scan_token();
}

typedef struct { char input[64]; } IdentCase;

ParameterizedTestParameters(lexer_identifier, valid_cases) {
  static const IdentCase cases[] = {
    { "a" }, { "x" }, { "foo" }, { "Foo" }, { "FOO" }, { "fOO" },
    { "aBc123" }, { "_" }, { "_a" }, { "_1" }, { "__x" }, { "a_b" },
    { "a_b_2" }, { "_x1" }, { "x_y_z99" }, { "a1b2c3" },
  };
  return cr_make_param_array(IdentCase, cases, 
      sizeof(cases) / sizeof(IdentCase));
}

ParameterizedTest(IdentCase *tc, lexer_identifier, valid_cases) {
  Token t = next_token(tc->input);
  cr_assert_eq(t.type, TOKEN_IDENTIFIER,
      "Error in '%s': expected IDENTIFIER but got %d", tc->input, t.type);
  cr_assert_eq(t.length, (i32)strlen(tc->input),
      "'%s' most tokenize completely (len %d, got %d)",
      tc->input, (i32)strlen(tc->input), t.length);
  Token e = scan_token();
  cr_assert_eq(e.type, TOKEN_EOF,
      "'%s' most be a SINGLE token ", tc->input);
}

typedef struct { char input[16]; TokenType expected; } CaseCase;

ParameterizedTestParameters(lexer_identifier, case_sensitive) {
  static const CaseCase cases[] = {
    { "if", TOKEN_IF }, { "If", TOKEN_IDENTIFIER },
    { "IF", TOKEN_IDENTIFIER }, { "iF", TOKEN_IDENTIFIER },
    { "Return", TOKEN_IDENTIFIER }, { "RETURN", TOKEN_IDENTIFIER },
    { "Auto", TOKEN_IDENTIFIER },
    { "include", TOKEN_INCLUDE }, { "INCLUDE", TOKEN_IDENTIFIER },
  };
  return cr_make_param_array(CaseCase, cases,
      sizeof(cases) / sizeof(CaseCase)
      );
}

ParameterizedTest(CaseCase *tc, lexer_identifier, case_sensitive) {
  Token t = next_token(tc->input);
  cr_assert_eq(t.type, tc->expected,
      "Error in '%s': expected %d but got %d",
      tc->input, tc->expected, t.type);
}

typedef struct {
  char input[32];
  TokenType expected[6];
} StreamCase;

ParameterizedTestParameters(lexer_identifier, streams) {
  static const StreamCase cases[] = {
    { "x+1",    { TOKEN_IDENTIFIER, TOKEN_PLUS, TOKEN_INTEGER_LITERAL, TOKEN_EOF } },
    { "x+++y",  { TOKEN_IDENTIFIER, TOKEN_PLUS_PLUS, TOKEN_PLUS, TOKEN_IDENTIFIER, TOKEN_EOF } },
    { "a&&b",   { TOKEN_IDENTIFIER, TOKEN_AND, TOKEN_IDENTIFIER, TOKEN_EOF } },
    { "x->y",   { TOKEN_IDENTIFIER, TOKEN_MEMBER, TOKEN_IDENTIFIER, TOKEN_EOF } },
    { "if(x)",  { TOKEN_IF, TOKEN_LEFT_PAREN, TOKEN_IDENTIFIER, TOKEN_RIGHT_PAREN, TOKEN_EOF } },
    { "return;",{ TOKEN_RETURN, TOKEN_SEMICOLON, TOKEN_EOF } },
    { "if-else",{ TOKEN_IF, TOKEN_MINUS, TOKEN_ELSE, TOKEN_EOF } },
    { "foo bar",{ TOKEN_IDENTIFIER, TOKEN_IDENTIFIER, TOKEN_EOF } },
    { "a /*c*/ b", { TOKEN_IDENTIFIER, TOKEN_IDENTIFIER, TOKEN_EOF } },
    { "1abc",   { TOKEN_INTEGER_LITERAL, TOKEN_IDENTIFIER, TOKEN_EOF } },
    { "1_",     { TOKEN_INTEGER_LITERAL, TOKEN_IDENTIFIER, TOKEN_EOF } },
    { "a1b2",   { TOKEN_IDENTIFIER, TOKEN_EOF } },
    { "ifx+1",  { TOKEN_IDENTIFIER, TOKEN_PLUS, TOKEN_INTEGER_LITERAL, TOKEN_EOF } },
  };
  return cr_make_param_array(StreamCase, cases,
      sizeof(cases) / sizeof(StreamCase)
      );
}

ParameterizedTest(StreamCase *tc, lexer_identifier, streams) {
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
      "'%s' most finish on EOF", tc->input
      );
}

Test(lexer_identifier, long_identifier) {
  char buf[201];
  memset(buf, 'a', 150);
  memset(buf + 150, '_', 49);
  buf[199] = 'b';
  buf[200] = '\0';

  Token t = next_token(buf);
  cr_assert_eq(t.type, TOKEN_IDENTIFIER);
  cr_assert_eq(t.length, 200, "len 200, got %d", t.length);
  Token e = scan_token();
  cr_assert_eq(e.type, TOKEN_EOF);
}

Test(lexer_identifier, line_numbers) {
  init_scan("foo\nbar");
  Token a = scan_token();
  cr_assert_eq(a.line, 1);
  Token b = scan_token();
  cr_assert_eq(b.line, 2);
}
