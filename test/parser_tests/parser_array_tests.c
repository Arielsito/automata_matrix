#include <assert.h>
#include <criterion/criterion.h>
#include <criterion/internal/test.h>
#include "parser_util.h"

TestSuite(parser_array, .init = NULL);

// simple arrs

Test(parser_array, one_dim_no_init) {
  assert_ast("int a[5];", "(program 1 (decl INT (a[(lit int 5)])))");
}

Test(parser_array, one_dim_with_var_size) {
  assert_ast("int a[x];", "(program 1 (decl INT (a[(var x)])))");
}

Test(parser_array, unsized_array) {
  assert_ast("int a[];", "(program 1 (decl INT (a[])))");
}

Test(parser_array, one_dim_with_init) {
  assert_ast("int a[5] = {1, 2};",
      "(program 1 (decl INT (a[(lit int 5)] { (lit int 1) (lit int 2) })))");
}

Test(parser_array, unsized_with_init) {
  assert_ast("int a[] = {1, 2};",
      "(program 1 (decl INT (a[] { (lit int 1) (lit int 2) })))");
}

Test(parser_array, empty_init) {
  assert_ast("int a[2] = {};", "(program 1 (decl INT (a[(lit int 2)] { })))");
}

Test(parser_array, many_initializers) {
  assert_ast("int a[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};",
      "(program 1 (decl INT (a[(lit int 10)] { (lit int 1) (lit int 2) (lit int 3)"
      " (lit int 4) (lit int 5) (lit int 6) (lit int 7) (lit int 8) (lit int 9)"
      " (lit int 10) (lit int 11) })))");
}

// h-dim arrs

Test(parser_array, two_dim_no_init) {
  assert_ast("int a[2][2];",
      "(program 1 (decl INT (a[(lit int 2)][(lit int 2)])))");
}

Test(parser_array, two_dim_nested_init) {
  assert_ast("int a[2][3] = {{1,2,3},{4,5,6}};",
      "(program 1 (decl INT (a[(lit int 2)][(lit int 3)]"
      " { { (lit int 1) (lit int 2) (lit int 3) }"
      " { (lit int 4) (lit int 5) (lit int 6) } })))");
}

Test(parser_array, two_dim_nested_init_spaced) {
  assert_ast("int a[2][3] = { {1,2,3}, {4,5,6} };",
      "(program 1 (decl INT (a[(lit int 2)][(lit int 3)]"
      " { { (lit int 1) (lit int 2) (lit int 3) }"
      " { (lit int 4) (lit int 5) (lit int 6) } })))");
}

Test(parser_array, two_dim_partial_init) {
  assert_ast("int a[2][3] = {{1,2,3}};",
      "(program 1 (decl INT (a[(lit int 2)][(lit int 3)]"
      " { { (lit int 1) (lit int 2) (lit int 3) } })))");
}

Test(parser_array, three_dim_no_init) {
  assert_ast("int a[2][3][4];",
      "(program 1 (decl INT (a[(lit int 2)][(lit int 3)][(lit int 4)])))");
}

// arrs w other decls
Test(parser_array, arr_and_val_declarators) {
  assert_ast("int a[3], b;", "(program 1 (decl INT (a[(lit int 3)]) (b)))");
}

Test(parser_array, arr_inside_block) {
  assert_ast("int x = 1; { int a[2] = {1, 2}; }", 
      "(program 2 (decl INT (x (lit int 1))) (block 1 (decl INT (a[(lit int 2)] { (lit int 1) (lit int 2) }))))");
}

// arr access

Test(parser_array, index_basic) {
  assert_ast("a[i];",
      "(program 1 (stmt (index (var a) (var i))))");
}

Test(parser_array, index_with_literal) {
  assert_ast("a[0];",
      "(program 1 (stmt (index (var a) (lit int 0))))");
}

Test(parser_array, index_with_expression) {
  assert_ast("a[1 + 2];",
      "(program 1 (stmt (index (var a) (bin PLUS (lit int 1) (lit int 2)))))");
}

Test(parser_array, index_nested) {
  assert_ast("a[i][j];",
      "(program 1 (stmt (index (index (var a) (var i)) (var j))))");
}

Test(parser_array, index_into_literal) {
  assert_ast("x = a[b[i]];",
      "(program 1 (stmt (assign EQUAL (var x) (index (var a) (index (var b) (var i))))))");
}

Test(parser_array, index_on_read) {
  assert_ast("x = a[i];",
      "(program 1 (stmt (assign EQUAL (var x) (index (var a) (var i)))))");
}

Test(parser_array, index_on_write) {
  assert_ast("a[i] = b[j];",
      "(program 1 (stmt (assign EQUAL (index (var a) (var i)) (index (var b) (var j)))))");
}

Test(parser_array, index_compound_assign) {
  assert_ast("suma += numeros[i];",
      "(program 1 (stmt (assign PLUS_EQUAL (var suma) (index (var numeros) (var i)))))");
}

Test(parser_array, index_postfix_inc) {
  assert_ast("a[i]++;",
      "(program 1 (stmt (unary post PLUS_PLUS (index (var a) (var i)))))");
}

Test(parser_array, index_in_condition) {
  assert_ast("if (a[i] == 0) b[0] = 1;",
      "(program 1 (if (bin EQUAL_EQUAL (index (var a) (var i)) (lit int 0))"
      " (stmt (assign EQUAL (index (var b) (lit int 0)) (lit int 1)))))");
}

Test(parser_array, index_assignment_invalid) {
  assert_parse_fails("a[0] = {1};");
}

Test(parser_array, index_missing_close) {
  assert_parse_fails("a[i;");
}
