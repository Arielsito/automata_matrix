#include <assert.h>
#include <criterion/criterion.h>
#include <criterion/internal/assert.h>
#include <criterion/internal/test.h>
#include "../include/lexer.h"

// initialize scanner
static void initialize_lexer(void) {
  init_lexer();
}
TestSuite(lexer_stream, .init = initialize_lexer);
static Token next_token(const char *src) {
  init_scan(src);
  return scan_token();
}

// to validate a full stream
static void assert_token_stream(const char *src, const TokenType *expected) {
  init_scan(src);
  for (i32 i = 0; expected[i] != TOKEN_EOF; i++) {
    Token t = scan_token();
    cr_assert_eq(t.type, expected[i],
      "Token %d of '%s': expected %d but got %d ('%.*s')",
      i, src, expected[i], t.type, t.length, t.start);
  }
  Token e = scan_token();
  cr_assert_eq(e.type, TOKEN_EOF,
      "'%s' must finish on EOF", src
      );
}

Test(lexer_stream, declaration_int) {
  static const TokenType expected[] = {
    TOKEN_INT, TOKEN_IDENTIFIER, TOKEN_EQUAL, TOKEN_INTEGER_LITERAL, TOKEN_SEMICOLON, TOKEN_EOF
  };

  assert_token_stream("int x = 5;", expected);
}

Test(lexer_stream, declaration_mixed_literals) {
  static const TokenType expected[] = {
    TOKEN_DOUBLE, TOKEN_IDENTIFIER, TOKEN_EQUAL, TOKEN_DOUBLE_LITERAL, TOKEN_SEMICOLON,
    TOKEN_CHAR, TOKEN_IDENTIFIER, TOKEN_EQUAL, TOKEN_CHAR_LITERAL, TOKEN_SEMICOLON,
    TOKEN_IDENTIFIER, TOKEN_EQUAL, TOKEN_STRING_LITERAL, TOKEN_SEMICOLON, TOKEN_EOF
  };
  assert_token_stream("double y = 1.5; char c = 'a'; s = \"hi\";", expected);
}

Test(lexer_stream, mini_function) {
  static const TokenType expected[] = {
    TOKEN_INT, TOKEN_IDENTIFIER, TOKEN_LEFT_PAREN, TOKEN_RIGHT_PAREN, TOKEN_LEFT_BRACE, TOKEN_IF,
    TOKEN_LEFT_PAREN, TOKEN_IDENTIFIER, TOKEN_RIGHT_PAREN, TOKEN_RETURN, TOKEN_INTEGER_LITERAL, 
    TOKEN_SEMICOLON, TOKEN_RIGHT_BRACE, TOKEN_EOF
  };
  assert_token_stream("int main() { if (x)  return 0; }", expected);
}
