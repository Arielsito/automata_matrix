#include <criterion/criterion.h>
#include <criterion/internal/assert.h>
#include <criterion/internal/test.h>
#include <criterion/parameterized.h>
#include "lexer.h"

// initialize scanner
static void initialize_lexer(void) {
  init_lexer();
}
TestSuite(lexer_comments_ws, .init = initialize_lexer);
static Token next_token(const char *src) {
  init_scan(src);
  return scan_token();
}

typedef struct {
  char input[24];
  TokenType expected[8];
} WsStreamCase;

ParameterizedTestParameters(lexer_comments_ws, line_comments) {
  static const WsStreamCase cases[] = {
    { "//x",    { TOKEN_EOF } },
    { "//x\ny", { TOKEN_IDENTIFIER, TOKEN_EOF } },
    { "a//b\nc",{ TOKEN_IDENTIFIER, TOKEN_IDENTIFIER, TOKEN_EOF } },
    { "a//=b",  { TOKEN_IDENTIFIER, TOKEN_EOF } },
    { "a//",    { TOKEN_IDENTIFIER, TOKEN_EOF } },
    { "a // b", { TOKEN_IDENTIFIER, TOKEN_EOF } },
    { "//x\nif",{ TOKEN_IF, TOKEN_EOF } },
  };
  return cr_make_param_array(WsStreamCase, cases, 
      sizeof(cases) / sizeof(WsStreamCase)
      );
}

ParameterizedTest(WsStreamCase *tc, lexer_comments_ws, line_comments) {
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

ParameterizedTestParameters(lexer_comments_ws, block_comments) {
  static const WsStreamCase cases[] = {
    { "/*c*/",    { TOKEN_EOF } },
    { "a/*b*/c",  { TOKEN_IDENTIFIER, TOKEN_IDENTIFIER, TOKEN_EOF } },
    { "a/**/b",   { TOKEN_IDENTIFIER, TOKEN_IDENTIFIER, TOKEN_EOF } },
    { "a/*",      { TOKEN_IDENTIFIER, TOKEN_EOF } },
    { "a/*b",     { TOKEN_IDENTIFIER, TOKEN_EOF } },
    { "x /* */ */", { TOKEN_IDENTIFIER, TOKEN_STAR, TOKEN_SLASH, TOKEN_EOF } },
    { "*/x",      { TOKEN_STAR, TOKEN_SLASH, TOKEN_IDENTIFIER, TOKEN_EOF } },
    { "/=x",      { TOKEN_SLASH_EQUAL, TOKEN_IDENTIFIER, TOKEN_EOF } },
    { "a/ b",     { TOKEN_IDENTIFIER, TOKEN_SLASH, TOKEN_IDENTIFIER, TOKEN_EOF } },
    { "/**/a",    { TOKEN_IDENTIFIER, TOKEN_EOF } },
  };
  return cr_make_param_array(WsStreamCase, cases, 
      sizeof(cases) / sizeof(WsStreamCase)
      );
}

ParameterizedTest(WsStreamCase *tc, lexer_comments_ws, block_comments) {
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

Test(lexer_comments_ws, comment_line_numbers) {
  init_scan("//x\ny");
  Token t = scan_token();
  cr_assert_eq(t.type, TOKEN_IDENTIFIER);
  cr_assert_eq(t.line, 2);

  init_scan("/*\n\n*/y");
  t = scan_token();
  cr_assert_eq(t.type, TOKEN_IDENTIFIER);
  cr_assert_eq(t.line, 3);

  init_scan("x/*\n*/y");
  t = scan_token();
  cr_assert_eq(t.type, TOKEN_IDENTIFIER);
  cr_assert_eq(t.line, 1);
  t = scan_token();
  cr_assert_eq(t.type, TOKEN_IDENTIFIER);
  cr_assert_eq(t.line, 2);
}

Test(lexer_comments_ws, comments_not_in_literals) {
  init_scan("x \"a//b\"");
  Token t = scan_token();
  cr_assert_eq(t.type, TOKEN_IDENTIFIER);
  t = scan_token();
  cr_assert_eq(t.type, TOKEN_STRING_LITERAL,
      "comment inside string must not be a comment itself (got %d)", t.type
      );
  cr_assert_eq(t.length, 6);

  init_scan("x \"a/*b*/c\"");
  t = scan_token();
  cr_assert_eq(t.type, TOKEN_IDENTIFIER);
  t = scan_token();
  cr_assert_eq(t.type, TOKEN_STRING_LITERAL,
      "comment inside string must not be a comment itself (got %d)", t.type
      );
  cr_assert_eq(t.length, 9);

  init_scan("'/'");
  t = scan_token();
  cr_assert_eq(t.type, TOKEN_CHAR_LITERAL,
      "comment inside char must not be a comment itself (got %d)", t.type
      );
}

ParameterizedTestParameters(lexer_comments_ws, whitespace) {
  static const WsStreamCase cases[] = {
    { "a\tb",      { TOKEN_IDENTIFIER, TOKEN_IDENTIFIER, TOKEN_EOF } },
    { "a\rb",      { TOKEN_IDENTIFIER, TOKEN_IDENTIFIER, TOKEN_EOF } },
    { "a\r\nb",    { TOKEN_IDENTIFIER, TOKEN_IDENTIFIER, TOKEN_EOF } },
    { "a\n b",     { TOKEN_IDENTIFIER, TOKEN_IDENTIFIER, TOKEN_EOF } },
    { "  a  ",     { TOKEN_IDENTIFIER, TOKEN_EOF } },
    { "a + + b",   { TOKEN_IDENTIFIER, TOKEN_PLUS, TOKEN_PLUS, TOKEN_IDENTIFIER, TOKEN_EOF } },
    { "a - -> b",  { TOKEN_IDENTIFIER, TOKEN_MINUS, TOKEN_MEMBER, TOKEN_IDENTIFIER, TOKEN_EOF } },
    { "a\t\r \nb", { TOKEN_IDENTIFIER, TOKEN_IDENTIFIER, TOKEN_EOF } },
    { "( a )",     { TOKEN_LEFT_PAREN, TOKEN_IDENTIFIER, TOKEN_RIGHT_PAREN, TOKEN_EOF } },
    { "a  b",      { TOKEN_IDENTIFIER, TOKEN_IDENTIFIER, TOKEN_EOF } },
  };
  return cr_make_param_array(WsStreamCase, cases,
      sizeof(cases) / sizeof(WsStreamCase)
      );
}

ParameterizedTest(WsStreamCase *tc, lexer_comments_ws, whitespace) {
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

ParameterizedTestParameters(lexer_comments_ws, invalid_whitespace) {
  static const WsStreamCase cases[] = {
    { "\v",    { TOKEN_ERROR, TOKEN_EOF } },
    { "\f",    { TOKEN_ERROR, TOKEN_EOF } },
    { "a\vb",  { TOKEN_IDENTIFIER, TOKEN_ERROR, TOKEN_IDENTIFIER, TOKEN_EOF } },
    { "a\fb",  { TOKEN_IDENTIFIER, TOKEN_ERROR, TOKEN_IDENTIFIER, TOKEN_EOF } },
    { "a \v b",{ TOKEN_IDENTIFIER, TOKEN_ERROR, TOKEN_IDENTIFIER, TOKEN_EOF } },
  };
  return cr_make_param_array(WsStreamCase, cases,
      sizeof(cases) / sizeof(WsStreamCase)
      );
}

ParameterizedTest(WsStreamCase *tc, lexer_comments_ws, invalid_whitespace) {
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

Test(lexer_comments_ws, whitespace_line_numbers) {
  init_scan("a\nb\nc");
  scan_token();
  Token t = scan_token();
  cr_assert_eq(t.line, 2);
  t = scan_token();
  cr_assert_eq(t.line, 3);

  init_scan("a\r\nb");
  scan_token();
  t = scan_token();
  cr_assert_eq(t.line, 2);

  init_scan("a\t\nb");
  scan_token();
  t = scan_token();
  cr_assert_eq(t.line, 2);
}
