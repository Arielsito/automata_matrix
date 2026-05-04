#include "../include/debug.h"
// TokenType values in strings for debugging
static const char* const token_names[] = {
  [TOKEN_IDENTIFIER]      = "IDENTIFIER",
  [TOKEN_DIGIT]           = "DIGIT",
  [TOKEN_INTEGER_LITERAL] = "INTEGER LITERAL",
  [TOKEN_DOUBLE_LITERAL]  = "DOUBLE LITERAL",
  [TOKEN_LETTER]          = "LETTER",
  [TOKEN_ERROR]           = "ERROR",
  [TOKEN_EOF]             = "EOF",
  [TOKEN_AUTO]     = "AUTO",     [TOKEN_BREAK]    = "BREAK",
  [TOKEN_CASE]     = "CASE",     [TOKEN_CHAR]     = "CHAR",
  [TOKEN_CONST]    = "CONST",    [TOKEN_CONTINUE] = "CONTINUE",
  [TOKEN_DEFAULT]  = "DEFAULT",  [TOKEN_DO]       = "DO",
  [TOKEN_DOUBLE]   = "DOUBLE",   [TOKEN_ELSE]     = "ELSE",
  [TOKEN_ENUM]     = "ENUM",     [TOKEN_EXTERN]   = "EXTERN",
  [TOKEN_FLOAT]    = "FLOAT",    [TOKEN_FOR]      = "FOR",
  [TOKEN_GOTO]     = "GOTO",     [TOKEN_IF]       = "IF",
  [TOKEN_INT]      = "INT",      [TOKEN_LONG]     = "LONG",
  [TOKEN_REGISTER] = "REGISTER", [TOKEN_RETURN]   = "RETURN",
  [TOKEN_SHORT]    = "SHORT",    [TOKEN_SIGNED]   = "SIGNED",
  [TOKEN_SIZEOF]   = "SIZEOF",   [TOKEN_STATIC]   = "STATIC",
  [TOKEN_STRUCT]   = "STRUCT",   [TOKEN_SWITCH]   = "SWITCH",
  [TOKEN_TYPEDEF]  = "TYPEDEF",  [TOKEN_UNION]    = "UNION",
  [TOKEN_UNSIGNED] = "UNSIGNED", [TOKEN_VOID]     = "VOID",
  [TOKEN_VOLATILE] = "VOLATILE", [TOKEN_WHILE]    = "WHILE",
};

const char* token_type_name(TokenType type) {
  return token_names[type];
}
