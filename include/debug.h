#ifndef DEBUG_H
#define DEBUG_H

#include "lexer.h"
#include "parser.h"
#include "semantic.h"

const char* token_type_name(TokenType);

bool ast_to_string(const AstNode*, char*, u32);

void semantic_print_tree(const AstNode*, TraversalOrder);

#endif
