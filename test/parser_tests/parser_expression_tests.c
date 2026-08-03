#include <criterion/criterion.h>
#include "parser_util.h"

TestSuite(parser_expr, .init = NULL);

// literals

Test(parser_expr, literal_int) {
  assert_ast("5;", "(program 1 (stmt (lit int 5)))");
}

Test(parser_expr, literal_zero) {
  assert_ast("0;", "(program 1 (stmt (lit int 0)))");
}

Test(parser_expr, literal_double) {
  assert_ast("1.5;", "(program 1 (stmt (lit double 1.5)))");
}

Test(parser_expr, literal_string) {
  assert_ast("\"hi\";", "(program 1 (stmt (lit string \"hi\")))");
}

Test(parser_expr, literal_char) {
  assert_ast("'a';", "(program 1 (stmt (lit char 'a')))");
}

// variables

Test(parser_expr, variable_solo) {
  assert_ast("x;", "(program 1 (stmt (var x)))");
}

// presedence

Test(parser_expr, add) {
  assert_ast("1 + 2;", "(program 1 (stmt (bin PLUS (lit int 1) (lit int 2))))");
}

Test(parser_expr, mul_binds_tighter_than_add) {
  assert_ast("1 + 2 * 3;",
    "(program 1 (stmt (bin PLUS (lit int 1) (bin STAR (lit int 2) (lit int 3)))))");
}

Test(parser_expr, add_left_assoc) {
  assert_ast("1 - 2 - 3;",
    "(program 1 (stmt (bin MINUS (bin MINUS (lit int 1) (lit int 2)) (lit int 3))))");
}

Test(parser_expr, parentheses_override) {
  assert_ast("(1 + 2) * 3;",
    "(program 1 (stmt (bin STAR (bin PLUS (lit int 1) (lit int 2)) (lit int 3))))");
}

// unary

Test(parser_expr, unary_minus) {
  assert_ast("-x;", "(program 1 (stmt (unary pre MINUS (var x))))");
}

Test(parser_expr, postfix_increment) {
  assert_ast("x++ ;", "(program 1 (stmt (unary post PLUS_PLUS (var x))))");
}

Test(parser_expr, not_flag) {
  assert_ast("!x;", "(program 1 (stmt (unary pre NOT (var x))))");
}

// comparison and logical

Test(parser_expr, comparison) {
  assert_ast("a < b;",
    "(program 1 (stmt (bin LESS (var a) (var b))))");
}

Test(parser_expr, logical_and) {
  assert_ast("a && b;",
    "(program 1 (stmt (bin AND (var a) (var b))))");
}

Test(parser_expr, and_binds_tighter_than_or) {
  assert_ast("a || b && c;",
    "(program 1 (stmt (bin OR (var a) (bin AND (var b) (var c)))))");
}

Test(parser_expr, equality) {
  assert_ast("a == b;",
    "(program 1 (stmt (bin EQUAL_EQUAL (var a) (var b))))");
}

// assignment

Test(parser_expr, assign_simple) {
  assert_ast("x = 5;", "(program 1 (stmt (assign EQUAL x (lit int 5))))");
}

Test(parser_expr, assign_additive) {
  assert_ast("x += 1;", "(program 1 (stmt (assign PLUS_EQUAL x (lit int 1))))");
}

Test(parser_expr, assign_left_assoc_to_var) {
  assert_ast("x -= y;", "(program 1 (stmt (assign MINUS_EQUAL x (var y))))");
}
