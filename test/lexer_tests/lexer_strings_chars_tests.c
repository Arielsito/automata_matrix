#include <criterion/criterion.h>
#include <criterion/internal/assert.h>
#include <criterion/internal/test.h>
#include <criterion/parameterized.h>
#include <stddef.h>
#include "../include/lexer.h"

// initialize scanner
static void initialize_lexer(void) {
  init_lexer();
}
TestSuite(lexer_s_c, .init = initialize_lexer);
static Token next_token(const char *src) {
  init_scan(src);
  return scan_token();
}

typedef struct { char input[32]; } LitCase;

ParameterizedTestParameters(lexer_s_c, string_literals) {
  static const LitCase cases[] = {
    { "\"\"" }, { "\"a\"" }, { "\"abc\"" }, { "\"a b c\"" }, { "\"hello world\"" },
    { "\"a\\\"b\"" }, { "\"a\\\\b\"" }, { "\"a\\nb\"" },
    { "\"\\t\\r\\0\\\"\\'\\\\\"" },
    { "\"a\\\nb\"" },
  };
  return cr_make_param_array(LitCase, cases, 
      sizeof(cases) / sizeof(LitCase)
      );
}

ParameterizedTest(LitCase *tc, lexer_s_c, string_literals) {
  Token t = next_token(tc->input);
  cr_assert_eq(t.type, TOKEN_STRING_LITERAL,
      "Error in '%s': expected STRING but got %d", tc->input, t.type);
  cr_assert_eq(t.length, (i32)strlen(tc->input),
      "'%s' most tokenize completely (len %d, got %d)",
      tc->input, (i32)strlen(tc->input), t.length);
  Token e = scan_token();
  cr_assert_eq(e.type, TOKEN_EOF,
      "'%s' most be a SINGLE token ", tc->input);
}

ParameterizedTestParameters(lexer_s_c, char_literals) {
  static const LitCase cases[] = {
    { "'a'" }, { "'0'" }, { "' '" }, { "'z'" },
    { "'\\n'" }, { "'\\t'" }, { "'\\r'" }, { "'\\0'" },
    { "'\\''" }, { "'\\\\'" }, { "'\\\"'" },

  };
  return cr_make_param_array(LitCase, cases, 
      sizeof(cases) / sizeof(LitCase)
      );
}

ParameterizedTest(LitCase *tc, lexer_s_c, char_literals) {
  Token t = next_token(tc->input);
  cr_assert_eq(t.type, TOKEN_CHAR_LITERAL,
      "Error in '%s': expected CHAR but got %d", tc->input, t.type);
  cr_assert_eq(t.length, (i32)strlen(tc->input),
      "'%s' most tokenize completely (len %d, got %d)",
      tc->input, (i32)strlen(tc->input), t.length);
  Token e = scan_token();
  cr_assert_eq(e.type, TOKEN_EOF,
      "'%s' most be a SINGLE token ", tc->input);
}

typedef struct {
  char input[24];
  char expected_msg[48];
} ErrCase;

ParameterizedTestParameters(lexer_s_c, string_errors) {
  static const ErrCase cases[] = {
    { "\"a", "Lexer: unterminated string literal." },
    { "\"", "Lexer: unterminated string literal." },
    { "\"a\nb\"", "Lexer: unterminated string literal." },
  };
  return cr_make_param_array(ErrCase, cases,
      sizeof(cases) / sizeof(ErrCase)
      );
}

ParameterizedTest(ErrCase *tc, lexer_s_c, string_errors) {
  Token t = next_token(tc->input);
  cr_assert_eq(t.type, TOKEN_ERROR,
      "Error in '%s': expected ERROR but got %d", tc->input, t.type
      );
  const char *msg = take_lexer_error();
  cr_assert_not_null(msg, "'%s' must left a message", tc->input);
  cr_assert_str_eq(msg, tc->expected_msg,
      "'%s': msg '%s', expected '%s'", tc->input, msg, tc->expected_msg
      );
}

ParameterizedTestParameters(lexer_s_c, char_errors) {
  static const ErrCase cases[] = {
    { "''", "Lexer: unterminated char literal." },
    { "'ab'", "Lexer: unterminated char literal." },
    { "'a", "Lexer: unterminated char literal." },
    { "'", "Lexer: unterminated char literal." },
  };
  return cr_make_param_array(ErrCase, cases,
      sizeof(cases) / sizeof(ErrCase)
      );
}

ParameterizedTest(ErrCase *tc, lexer_s_c, char_errors) {
  Token t = next_token(tc->input);
  cr_assert_eq(t.type, TOKEN_ERROR,
      "Error in '%s': expected ERROR but got %d", tc->input, t.type
      );
  const char *msg = take_lexer_error();
  cr_assert_not_null(msg, "'%s' must left a message", tc->input);
  cr_assert_str_eq(msg, tc->expected_msg,
      "'%s': msg '%s', expected '%s'", tc->input, msg, tc->expected_msg
      );
}

typedef struct {
  char input[32];
  TokenType expected[6];
} LitStreamCase;

ParameterizedTestParameters(lexer_s_c, string_char_streams) {
  static const LitStreamCase cases[] = {
     { "\"ab\" 'c'",
      { TOKEN_STRING_LITERAL, TOKEN_CHAR_LITERAL, TOKEN_EOF } },
    { "s = \"x\";",
      { TOKEN_IDENTIFIER, TOKEN_EQUAL, TOKEN_STRING_LITERAL, TOKEN_SEMICOLON, TOKEN_EOF } },
    { "'a'+'b'",
      { TOKEN_CHAR_LITERAL, TOKEN_PLUS, TOKEN_CHAR_LITERAL, TOKEN_EOF } },
  };
  return cr_make_param_array(LitStreamCase, cases,
      sizeof(cases) / sizeof(LitStreamCase)
      );
}

ParameterizedTest(LitStreamCase *tc, lexer_s_c, string_char_streams) {
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
      "'%s' most end with EOF", tc->input
      );
}

Test(lexer_s_c, string_at_eof) {
  Token t = next_token("\"abc\"");
  cr_assert_eq(t.type, TOKEN_STRING_LITERAL,
      "must not be unterminated (got %d)", t.type
      );
  cr_assert_eq(t.length, 5);
}

Test(lexer_s_c, char_at_eof) {
  Token t = next_token("'a'");
  cr_assert_eq(t.type, TOKEN_CHAR_LITERAL);
  cr_assert_eq(t.length, 3);
}

Test(lexer_s_c, unknown_escape_passes_lexer) {
  Token t = next_token("'\\q'");
  cr_assert_eq(t.type, TOKEN_CHAR_LITERAL,
      "Lexer allows any escape, parser rejects '%.*s'", t.length, t.start
      );
}

Test(lexer_s_c, error_message_clears) {
  init_scan("\"abc");
  Token t = scan_token();
  cr_assert_eq(t.type, TOKEN_ERROR);
  cr_assert_not_null(take_lexer_error());
  cr_assert_null(take_lexer_error(), "the message must be cleared after take_lexer_error");

  init_scan("x");
  Token ok = scan_token();
  cr_assert_eq(ok.type, TOKEN_IDENTIFIER);
  cr_assert_null(take_lexer_error(), "a valid token doesn't leave a message");
}
