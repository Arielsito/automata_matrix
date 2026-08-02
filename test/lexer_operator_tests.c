#include <criterion/criterion.h>
#include <criterion/parameterized.h>
#include "../include/lexer.h"
// initialize scanner
static void initialize_lexer(void) {
  init_lexer();
}
TestSuite(lexer_op, .init = initialize_lexer);
static Token next_token(const char *src) {
  init_scan(src);
  return scan_token();
}

typedef struct {
  char input[4];
  TokenType expected;
} OpCase;

ParameterizedTestParameters(lexer_op, operators) {
  static const OpCase cases[] = {
    { "~", TOKEN_NOT_BITW }, { "?", TOKEN_QUESTION }, { ":", TOKEN_COLON },
    { ",", TOKEN_COMMA }, { ";", TOKEN_SEMICOLON }, { ".", TOKEN_DOT },
    { "(", TOKEN_LEFT_PAREN }, { ")", TOKEN_RIGHT_PAREN },
    { "[", TOKEN_LEFT_BRACKET }, { "]", TOKEN_RIGHT_BRACKET },
    { "{", TOKEN_LEFT_BRACE }, { "}", TOKEN_RIGHT_BRACE },
    { "#", TOKEN_TAG },
    { "+", TOKEN_PLUS }, { "-", TOKEN_MINUS }, { "*", TOKEN_STAR },
    { "/", TOKEN_SLASH }, { "%", TOKEN_MOD },
    { "++", TOKEN_PLUS_PLUS }, { "--", TOKEN_MINUS_MINUS },
    { "=", TOKEN_EQUAL },
    { "+=", TOKEN_PLUS_EQUAL }, { "-=", TOKEN_MINUS_EQUAL },
    { "*=", TOKEN_STAR_EQUAL }, { "/=", TOKEN_SLASH_EQUAL }, { "%=", TOKEN_MOD_EQUAL },
    { "<<=", TOKEN_LSE }, { ">>=", TOKEN_RSE },
    { "&=", TOKEN_AND_BITW_EQUAL }, { "|=", TOKEN_OR_BITW_EQUAL },
    { "^=", TOKEN_XOR_BITW_EQUAL },
    { "==", TOKEN_EQUAL_EQUAL }, { "!=", TOKEN_NOT_EQUAL },
    { "<", TOKEN_LESS }, { "<=", TOKEN_LESS_EQUAL },
    { ">", TOKEN_GREATER }, { ">=", TOKEN_GREATER_EQUAL },
    { "!", TOKEN_NOT }, { "&&", TOKEN_AND }, { "||", TOKEN_OR },
    { "&", TOKEN_AND_BITW }, { "|", TOKEN_OR_BITW }, { "^", TOKEN_XOR_BITW },
    { "<<", TOKEN_LEFT_SHIFT }, { ">>", TOKEN_RIGHT_SHIFT },
    { "->", TOKEN_MEMBER },
  };
  return cr_make_param_array(OpCase, cases, 
      sizeof(cases) / sizeof(OpCase)
      );
}

ParameterizedTest(OpCase *tc, lexer_op, operators) {
  Token t = next_token(tc->input);
  cr_assert_eq(t.type, tc->expected,
      "Error in operator '%s': expected '%d' but got '%d'", tc->input, tc->expected, t.type
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
  char input[16];
  TokenType expected[7];
} MunchCase;

ParameterizedTestParameters(lexer_op, max_munch) {
  static const MunchCase cases[] = {
    { "a<<<b", { TOKEN_IDENTIFIER, TOKEN_LEFT_SHIFT, TOKEN_LESS,
                 TOKEN_IDENTIFIER, TOKEN_EOF } },
    { "a<<==b",{ TOKEN_IDENTIFIER, TOKEN_LSE, TOKEN_EQUAL,
                 TOKEN_IDENTIFIER, TOKEN_EOF } },
    { "x++++y",{ TOKEN_IDENTIFIER, TOKEN_PLUS_PLUS, TOKEN_PLUS_PLUS,
                 TOKEN_IDENTIFIER, TOKEN_EOF } },
    { "x-->y", { TOKEN_IDENTIFIER, TOKEN_MINUS_MINUS, TOKEN_GREATER,
                 TOKEN_IDENTIFIER, TOKEN_EOF } },
    { "a&&&b", { TOKEN_IDENTIFIER, TOKEN_AND, TOKEN_AND_BITW,
                 TOKEN_IDENTIFIER, TOKEN_EOF } },
    { "a|||b", { TOKEN_IDENTIFIER, TOKEN_OR, TOKEN_OR_BITW,
                 TOKEN_IDENTIFIER, TOKEN_EOF } },
    { "a!==b", { TOKEN_IDENTIFIER, TOKEN_NOT_EQUAL, TOKEN_EQUAL,
                 TOKEN_IDENTIFIER, TOKEN_EOF } },
    { "a<=>b", { TOKEN_IDENTIFIER, TOKEN_LESS_EQUAL, TOKEN_GREATER,
                 TOKEN_IDENTIFIER, TOKEN_EOF } },
    { "a=>b",  { TOKEN_IDENTIFIER, TOKEN_EQUAL, TOKEN_GREATER,
                 TOKEN_IDENTIFIER, TOKEN_EOF } },
    { "a=-b",  { TOKEN_IDENTIFIER, TOKEN_EQUAL, TOKEN_MINUS,
                 TOKEN_IDENTIFIER, TOKEN_EOF } },
    { "x>>>y", { TOKEN_IDENTIFIER, TOKEN_RIGHT_SHIFT, TOKEN_GREATER,
                 TOKEN_IDENTIFIER, TOKEN_EOF } },
    { "a&&b&c",{ TOKEN_IDENTIFIER, TOKEN_AND, TOKEN_IDENTIFIER,
                 TOKEN_AND_BITW, TOKEN_IDENTIFIER, TOKEN_EOF } },
    { "1<<2",  { TOKEN_INTEGER_LITERAL, TOKEN_LEFT_SHIFT,
                 TOKEN_INTEGER_LITERAL, TOKEN_EOF } },
    { "#define", { TOKEN_TAG, TOKEN_IDENTIFIER, TOKEN_EOF } },
    { "a+=b",  { TOKEN_IDENTIFIER, TOKEN_PLUS_EQUAL,
                 TOKEN_IDENTIFIER, TOKEN_EOF } },
    { "((x))", { TOKEN_LEFT_PAREN, TOKEN_LEFT_PAREN, TOKEN_IDENTIFIER,
                 TOKEN_RIGHT_PAREN, TOKEN_RIGHT_PAREN, TOKEN_EOF } },
  };
  return cr_make_param_array(MunchCase, cases,
      sizeof(cases) / sizeof(MunchCase)
      );
}

ParameterizedTest(MunchCase *tc, lexer_op, max_munch) {
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

Test(lexer_op, adjacent_operators) {
  init_scan("+-*/%=");
  TokenType expected[] = {
    TOKEN_PLUS, TOKEN_MINUS, TOKEN_STAR, TOKEN_SLASH, TOKEN_MOD_EQUAL
  };
  for (i32 i = 0; i < 5; i++) {
    Token t = scan_token();
    cr_assert_eq(t.type, expected[i],
        "Token %d: expected %d but got %d ('%.*s')",
        i, expected[i], t.type, t.length, t.start
        );
  }
  Token e = scan_token();
  cr_assert_eq(e.type, TOKEN_EOF);
}

Test(lexer_op, operator_at_eof) {
  Token t = next_token("->");
  cr_assert_eq(t.type, TOKEN_MEMBER,
      "'->' at EOF must be MEMBER, got (%d)", t.type
      );
  cr_assert_eq(t.length, 2);
}
