#ifndef DEBUG_H
#define DEBUG_H

#include "common.h"
#include "lexer.h"
#include "parser.h"
#include "ir.h"

const char* token_type_name(TokenType);

bool ast_to_string(const AstNode*, char*, u32);

void print_quadruples(const QuadrupleList*);

#endif
