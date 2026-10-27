#ifndef TYPE_RULES_H
#define TYPE_RULES_H

#include "lexer.h"
#include "parser.h"
#include "semantic_type.h"

bool is_integer_type(SemanticType);
bool is_arithmetic_type(SemanticType);
SemanticType declared_type(TypeBase);
SemanticType integer_promotion(SemanticType);
SemanticType default_argument_promotion(SemanticType);
SemanticType common_arithmetic_type(SemanticType, SemanticType);
bool can_assign_type(SemanticType, SemanticType);
TokenType compound_binary_operator(TokenType);
SemanticType binary_result_type(TokenType, SemanticType, SemanticType);

#endif
