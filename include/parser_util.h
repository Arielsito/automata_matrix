#ifndef PARSER_UTIL_H
#define PARSER_UTIL_H

#include "parser.h"
#include "debug.h"
#include <criterion/criterion.h>
#include <criterion/internal/assert.h>

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

static inline i32 statement_line(const char *src, u32 index) {
  AstNode *root = compile(src);
  cr_assert(root != NULL, "Expected '%s' to parse but it failed.", src);
  cr_assert(index < (u32)root->as.program.count,
      "Statement index %u out of range (program has %d).", index, root->as.program.count
      );
  return root->as.program.statements[index]->line;
}

static inline void assert_statement_line(const char *src, u32 index, i32 expected) {
  cr_assert_eq(statement_line(src, index), expected,
      "Line mismatch for statement %u of '%s': expected %d, got %d", index, src, expected, statement_line(src, index)
      );
}

#endif // PARSER_UTIL_H
