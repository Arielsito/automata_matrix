#include <criterion/criterion.h>
#include <criterion/internal/test.h>
#include "parser_util.h"

TestSuite(parser_control, .init = NULL);

// if / else

Test(parser_control, if_wo_else) {
  assert_ast("if (a) b;", "(program 1 (if (var a) (stmt (var b))))");
}

Test(parser_control, if_w_else) {
  assert_ast("if (a) b; else c;", 
      "(program 1 (if (var a) (stmt (var b)) (stmt (var c))))");
}

Test(parser_control, if_else_if_chain) {
  assert_ast("if (a) b; else if (c) d; else e;", 
      "(program 1 (if (var a) (stmt (var b)) (if (var c) (stmt (var d)) (stmt (var e)))))");
}

Test(parser_control, else_binds_to_nearest_if) {
  assert_ast("if (a) if(b) c; else d;", 
      "(program 1 (if (var a) (if (var b) (stmt (var c)) (stmt (var d)))))");
}

Test(parser_control, if_w_block) {
  assert_ast("if (a) { b; c; }", 
      "(program 1 (if (var a) (block 2 (stmt (var b)) (stmt (var c)))))");
}

Test(parser_control, if_empty_body_semicolon) {
  assert_ast("if (a);", "(program 1 (if (var a) (stmt nil)))");
}

// while

Test(parser_control, while_loop) {
  assert_ast("while (a < b) a += b;", 
      "(program 1 (while (bin LESS (var a) (var b)) (stmt (assign PLUS_EQUAL a (var b)))))");
}

Test(parser_control, while_block) {
  assert_ast("while (a) { a = a - 1; }", 
      "(program 1 (while (var a) (block 1 (stmt (assign EQUAL a (bin MINUS (var a) (lit int 1)))))))");
}

Test(parser_control, do_while) {
  assert_ast("do a; while(c != 2);", 
      "(program 1 (do-while (stmt (var a)) (bin NOT_EQUAL (var c) (lit int 2))))");
}

// for
Test(parser_control, for_full) {
  assert_ast("for (i = 0; i < 5; i++) a = i;", 
      "(program 1 (for (stmt (assign EQUAL i (lit int 0))) (bin LESS (var i) (lit int 5)) (unary post PLUS_PLUS (var i)) (stmt (assign EQUAL a (var i)))))");
}

Test(parser_control, for_infinite) {
  assert_ast("for (;;) a = 0;", "(program 1 (for nil nil nil (stmt (assign EQUAL a (lit int 0)))))");
}

// return / break / continue
Test(parser_control, return_w_val) {
  assert_ast("return 0;", "(program 1 (return (lit int 0)))");
}

Test(parser_control, return_wo_val) {
  assert_ast("return;", "(program 1 (return))");
}

Test(parser_control, break_stmt) {
  assert_ast("break;", "(program 1 (break))");
}

Test(parser_control, continue_stmt) {
  assert_ast("continue;", "(program 1 (continue))");
}

// blocks

Test(parser_control, empty_block) {
  assert_ast("{}", "(program 1 (block 0))");
}

Test(parser_control, block_multiple_stmts) {
  assert_ast("{ x = 1; y = 2; }", 
      "(program 1 (block 2 (stmt (assign EQUAL x (lit int 1))) (stmt (assign EQUAL y (lit int 2)))))");
}

// switch
Test(parser_control, switch_basic) {
  assert_ast("switch(x) { case 1: a = 5; break; }", 
      "(program 1 (switch (var x) (case (lit int 1) (stmt (assign EQUAL a (lit int 5))) (break))))");
}

Test(parser_control, switch_fallthrough) {
  assert_ast("switch (x) { case 1: case 2: b = 3; default: c = 0; }", 
      "(program 1 (switch (var x) (case (lit int 1)) (case (lit int 2) (stmt (assign EQUAL b (lit int 3)))) (default (stmt (assign EQUAL c (lit int 0))))))");
}

// nested control flow
Test(parser_control, nested_control_flow) {
  assert_ast("if (a) { base = 10; } else base = -limit; while (a < b) a += b;", 
      "(program 2 (if (var a) (block 1 (stmt (assign EQUAL base (lit int 10)))) (stmt (assign EQUAL base (unary pre MINUS (var limit))))) (while (bin LESS (var a) (var b)) (stmt (assign PLUS_EQUAL a (var b)))))");
}
