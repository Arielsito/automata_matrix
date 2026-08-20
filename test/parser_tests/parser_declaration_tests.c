#include <criterion/criterion.h>
#include <criterion/internal/test.h>
#include "parser_util.h"

TestSuite(parser_decl, .init = NULL);

// basic types

Test(parser_decl, int_no_init) {
  assert_ast("int x;", "(program 1 (decl INT (x)))");
}

Test(parser_decl, int_with_init) {
  assert_ast("int x = 5;", "(program 1 (decl INT (x (lit int 5))))");
}

Test(parser_decl, int_with_expr_init) {
  assert_ast("int x = a + 1;",
      "(program 1 (decl INT (x (bin PLUS (var a) (lit int 1)))))");
}

Test(parser_decl, double_no_init) {
  assert_ast("double y;", "(program 1 (decl DOUBLE (y)))");
}

Test(parser_decl, double_with_init) {
  assert_ast("double y = 2.5;", "(program 1 (decl DOUBLE (y (lit double 2.5))))");
}

Test(parser_decl, char_no_init) {
  assert_ast("char c;", "(program 1 (decl CHAR (c)))");
}

Test(parser_decl, char_with_init) {
  assert_ast("char c = 'a';", "(program 1 (decl CHAR (c (lit char 'a'))))");
}

Test(parser_decl, float_no_init) {
  assert_ast("float f;", "(program 1 (decl FLOAT (f)))");
}

Test(parser_decl, float_with_init) {
  assert_ast("float f = 1.5;", "(program 1 (decl FLOAT (f (lit double 1.5))))");
}

// multiple declarators

Test(parser_decl, two_declarators) {
  assert_ast("int a, b;", "(program 1 (decl INT (a) (b)))");
}

Test(parser_decl, two_declarators_one_with_init) {
  assert_ast("int a, b = 1;", "(program 1 (decl INT (a) (b (lit int 1))))");
}

Test(parser_decl, three_declarators_mixed) {
  assert_ast("int a = 1, b, c = 3;",
      "(program 1 (decl INT (a (lit int 1)) (b) (c (lit int 3))))");
}

// pointer declarators

Test(parser_decl, single_pointer) {
  assert_ast("int *p;", "(program 1 (decl INT (* p)))");
}

Test(parser_decl, double_pointer) {
  assert_ast("int **pp;", "(program 1 (decl INT (** pp)))");
}

Test(parser_decl, pointer_mixed_with_value) {
  assert_ast("int a, *p;", "(program 1 (decl INT (a) (* p)))");
}

Test(parser_decl, pointer_with_init) {
  assert_ast("int *p = 0;", "(program 1 (decl INT (* p (lit int 0))))");
}

// declarations and control flow

Test(parser_decl, multiple_decl_statements) {
  assert_ast("int a;\ndouble b;",
      "(program 2 (decl INT (a)) (decl DOUBLE (b)))");
}

Test(parser_decl, declaration_and_statement_mix) {
  assert_ast("int a = 2;\na = a * 2;",
      "(program 2 (decl INT (a (lit int 2)))"
      " (stmt (assign EQUAL (var a) (bin STAR (var a) (lit int 2)))))");
}

Test(parser_decl, declaration_inside_block) {
  assert_ast("if (x) { int y = 1; }",
      "(program 1 (if (var x) (block 1 (decl INT (y (lit int 1))))))");
}

Test(parser_decl, declaration_inside_while) {
  assert_ast("while (a) { int t = a; a = a - 1; }",
      "(program 1 (while (var a)"
      " (block 2 (decl INT (t (var a)))"
      " (stmt (assign EQUAL (var a) (bin MINUS (var a) (lit int 1)))))))");
}

// type prefixes

Test(parser_decl, unsigned_int) {
  assert_ast("unsigned int x;", "(program 1 (decl UNSIGNED INT (x)))");
}

Test(parser_decl, long_int) {
  assert_ast("long int x;", "(program 1 (decl LONG INT (x)))");
}

Test(parser_decl, long_alone) {
  assert_ast("long x;", "(program 1 (decl LONG INT (x)))");
}

Test(parser_decl, short_int) {
  assert_ast("short int s;", "(program 1 (decl SHORT INT (s)))");
}

Test(parser_decl, short_alone) {
  assert_ast("short s;", "(program 1 (decl SHORT INT (s)))");
}

Test(parser_decl, unsigned_char) {
  assert_ast("unsigned char c;", "(program 1 (decl UNSIGNED CHAR (c)))");
}

Test(parser_decl, long_long) {
  assert_parse_fails("long long x;");
}

Test(parser_decl, unsigned_long) {
  assert_ast("unsigned long l;", "(program 1 (decl UNSIGNED LONG INT (l)))");
}

Test(parser_decl, signed_int) {
  assert_ast("signed int x;", "(program 1 (decl SIGNED INT (x)))");
}

Test(parser_decl, long_double) {
  assert_ast("long double d;", "(program 1 (decl LONG DOUBLE (d)))");
}
