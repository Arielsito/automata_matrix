#include <criterion/criterion.h>
#include "parser_util.h"

TestSuite(parser_func, .init = NULL);

Test(parser_func, prototype_with_named_params) {
  assert_ast("int foo(int a, int b);",
             "(program 1 (function INT foo ((decl INT (a)) (decl INT (b)))))");
}

Test(parser_func, definition_with_body) {
  assert_ast("int add(int a, int b) { return a + b; }",
             "(program 1 (function INT add ((decl INT (a)) (decl INT (b))) (block 1 (return (bin PLUS (var a) (var b))))))");
}

Test(parser_func, void_return_void_params) {
  assert_ast("void bar(void);",
             "(program 1 (function VOID bar ()))");
}

Test(parser_func, void_return_empty_params) {
  assert_ast("void baz();",
             "(program 1 (function VOID baz ()))");
}

Test(parser_func, unsigned_param) {
  assert_ast("int f(unsigned int x);",
             "(program 1 (function INT f ((decl UNSIGNED INT (x)))))");
}

Test(parser_func, main_void_definition) {
  assert_ast("int main(void) { return 0; }",
             "(program 1 (function INT main () (block 1 (return (lit int 0)))))");
}

Test(parser_func, local_decl_in_body) {
  assert_ast("int foo(int a) { int x; return x; }",
             "(program 1 (function INT foo ((decl INT (a))) (block 2 (decl INT (x)) (return (var x)))))");
}

Test(parser_func, anonymous_params_in_prototype) {
  assert_ast("int foo(int, int);",
             "(program 1 (function INT foo ((decl INT) (decl INT))))");
}

Test(parser_func, named_pointer_param) {
  assert_ast("int q(int *a) { return 0; }",
             "(program 1 (function INT q ((decl INT (* a))) (block 1 (return (lit int 0)))))");
}

Test(parser_func, array_param) {
  assert_ast("int r(int a[5]) { return 0; }",
             "(program 1 (function INT r ((decl INT (a[(lit int 5)]))) (block 1 (return (lit int 0)))))");
}

Test(parser_func, definition_rejects_anonymous_param) {
  assert_parse_fails("int foo(int) { return 0; }");
}

Test(parser_func, definition_rejects_anonymous_pointer_param) {
  assert_parse_fails("int foo(int *) { return 0; }");
}
