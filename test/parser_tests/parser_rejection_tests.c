#include <criterion/criterion.h>
#include <criterion/internal/test.h>
#include "parser_util.h"

TestSuite(parser_reject, .init = NULL);

// binary

Test(parser_reject, binary_missing_operand) {
  assert_parse_fails("a = b + * 6;");
}

Test(parser_reject, trailing_binary_op) {
  assert_parse_fails("5 +");
}

Test(parser_reject, two_binary_ops_in_row) {
  assert_parse_fails("1 + * 2");
}

// assign to non-lvalue

Test(parser_reject, assign_to_literal) {
  assert_parse_fails("8 = limite;");
}

Test(parser_reject, assign_to_literal_expr) {
  assert_parse_fails("2 = @a + 4@ *@ 2@@@@@@@");
}

Test(parser_reject, missing_assign_rhs) {
  assert_parse_fails("x =");
}

// grouping

Test(parser_reject, unclosed_paren) {
  assert_parse_fails("(1 + 2");
}

Test(parser_reject, empty_grouping) {
  assert_parse_fails("( )");
}

Test(parser_reject, incomplete_grouping) {
  assert_parse_fails("(1 + )");
}

// statements

Test(parser_reject, stray_tokens) {
  assert_parse_fails("a b c;");
}

Test(parser_reject, missing_semicolon) {
  assert_parse_fails("1 + 2");
}

Test(parser_reject, missing_condition) {
  assert_parse_fails("if (a)");
}

Test(parser_reject, unclosed_while) {
  assert_parse_fails("while (a");
}

Test(parser_reject, unclosed_switch) {
  assert_parse_fails("switch (x) { case 1: a = 5;");
}

Test(parser_reject, dangling_case) {
  assert_parse_fails("case 1: a = 5;");
}

Test(parser_reject, dangling_else) {
  assert_parse_fails("else a = 5;");
}

Test(parser_reject, lone_binary_op) {
  assert_parse_fails("*");
}

Test(parser_reject, lone_unary_op) {
  assert_parse_fails("++");
}

