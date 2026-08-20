#include <criterion/assert.h>
#include <criterion/criterion.h>
#include <stdio.h>
#include "debug.h"
#include "parser.h"
#include "parser_util.h"

TestSuite(parser_stream, .init = NULL);

Test(parser_stream, two_stmts) {
  assert_ast("1;\n2;", "(program 2 (stmt (lit int 1)) (stmt (lit int 2)))");
}

Test(parser_stream, three_stmts) {
  assert_ast("a = 1; b = 2; c = a + b;", 
      "(program 3 (stmt (assign EQUAL (var a) (lit int 1))) (stmt (assign EQUAL (var b) (lit int 2))) (stmt (assign EQUAL (var c) (bin PLUS (var a) (var b)))))");
}

Test(parser_stream, mixed_control_and_expr) {
  assert_ast("i = 0;\nwhile (i < 3) i++;\nreturn i;",
      "(program 3 (stmt (assign EQUAL (var i) (lit int 0))) (while (bin LESS (var i) (lit int 3)) (stmt (unary post PLUS_PLUS (var i)))) (return (var i)))");
}

Test(parser_stream, program_with_block_and_if) {
  assert_ast("x = 1;\nif (x) { y = 2; }\nz = 3;",
      "(program 3 (stmt (assign EQUAL (var x) (lit int 1))) (if (var x) (block 1 (stmt (assign EQUAL (var y) (lit int 2))))) (stmt (assign EQUAL (var z) (lit int 3))))");
}

Test(parser_stream, many_statements) {
  assert_ast("a; b; c; d; e; f; g; h;",
      "(program 8 (stmt (var a)) (stmt (var b)) (stmt (var c)) (stmt (var d)) (stmt (var e)) (stmt (var f)) (stmt (var g)) (stmt (var h)))");
}

Test(parser_stream, growth_over_capacity) {
  char src[32758];
  char expected[65536];

  char *p = src;
  char *e = expected;
  for (u32 i = 0; i < 300; i++) {
    p += sprintf(p, "v%u = %u;", i, i);
    e += sprintf(e, " (stmt (assign EQUAL (var v%u) (lit int %u)))", i, i);
  }
  *e = '\0';
  char full[65536];
  snprintf(full, sizeof(full), "(program %u%s)", 300, expected);

  AstNode *root = compile(src);
  cr_assert(root != NULL, "Expected long program to parse.");
  char out[65536];
  bool ok = ast_to_string(root, out, sizeof(out));
  cr_assert(ok, "AST buffer too small for long program.");
  cr_assert_str_eq(out, full, "AST mismatch for long program\n expected:%s\n got:\t%s", full, out);
}
