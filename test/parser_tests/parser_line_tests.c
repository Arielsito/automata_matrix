#include <criterion/criterion.h>
#include "parser_util.h"

TestSuite(parser_line, .init = NULL);

Test(parser_line, first_statement_line_1) {
  assert_statement_line("1;", 0, 1);
}

Test(parser_line, second_line_after_newline) {
  assert_statement_line("1;\n2;", 1, 2);
}

Test(parser_line, three_lines) {
  assert_statement_line("a;\nb;\nc;", 2, 3);
}

Test(parser_line, statement_after_comment_and_newline) {
  assert_statement_line("// comment\n1;\n2;", 0, 2);
}

Test(parser_line, blank_lines_between) {
  assert_statement_line("1;\n\n\n2;", 1, 4);
}

Test(parser_line, line_of_variable) {
  AstNode *root = compile("x;\n");
  cr_assert(root != NULL);
  cr_assert_eq(root->as.program.statements[0]->line, 1);
}

Test(parser_line, for_loop_line) {
  assert_statement_line("for (i = 0; i < 5; i++) a = i;", 0, 1);
}

Test(parser_line, statement_after_two_lines) {
  assert_statement_line("if (a) b;\n\nc;", 1, 3);
}

Test(parser_line, if_condition_on_second_line) {
  assert_statement_line("if\n(a)\nb;", 0, 1);
}
