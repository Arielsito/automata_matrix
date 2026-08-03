#ifndef PARSER_UTIL_H
#define PARSER_UTIL_H

#include "../include/parser.h"
#include "../include/debug.h"
#include <criterion/criterion.h>

static inline bool parse_and_render(const char *src, char *out, u32 size) {
  AstNode *root = compile(src);
  if (root == NULL) return false;
  bool ok = ast_to_string(root, out, size);
  cr_assert(ok, "AST buffer too small for '%s'.", src);
  return true;
}

static inline void assert_ast(const char *src, const char *expected) {
  char out[2048];
  bool compiled = parse_and_render(src, out, sizeof(out));
  cr_assert(compiled, "Expected '%s' to parse but it failed.", src);
  cr_assert_str_eq(out, expected,
      "AST mismatch for '%s'\n expected: %s\n got:\t%s",
      src, expected, out
      );
}

static inline void assert_parse_fails(const char *src) {
  AstNode *root = compile(src);
  cr_assert(root == NULL, "Expected '%s' to fail, but it parsed.", src);
}

#endif // PARSER_UTIL_H
