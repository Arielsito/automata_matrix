#include "debug.h"
#include "lexer.h"
#include "parser.h"
#include <stdarg.h>
#include <stddef.h>
// TokenType values in strings for debugging
static const char* const token_names[] = {
  [TOKEN_IDENTIFIER]      = "IDENTIFIER",
  [TOKEN_DIGIT]           = "DIGIT",
  [TOKEN_INTEGER_LITERAL] = "INTEGER LITERAL",
  [TOKEN_DOUBLE_LITERAL]  = "DOUBLE LITERAL",
  [TOKEN_STRING_LITERAL]  = "STRING LITERAL",
  [TOKEN_CHAR_LITERAL]    = "CHAR LITERAL",
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
  [TOKEN_INCLUDE] = "INCLUDE",
  [TOKEN_LEFT_PAREN]    = "LEFT_PAREN",    [TOKEN_RIGHT_PAREN]   = "RIGHT_PAREN",
  [TOKEN_LEFT_BRACE]    = "LEFT_BRACE",    [TOKEN_RIGHT_BRACE]   = "RIGHT_BRACE",
  [TOKEN_LEFT_BRACKET]  = "LEFT_BRACKET",  [TOKEN_RIGHT_BRACKET] = "RIGHT_BRACKET",
  [TOKEN_COMMA]         = "COMMA",         [TOKEN_DOT]           = "DOT",
  [TOKEN_MINUS]         = "MINUS",         [TOKEN_PLUS]          = "PLUS",
  [TOKEN_MINUS_MINUS]   = "MINUS_MINUS",   [TOKEN_PLUS_PLUS]     = "PLUS_PLUS",
  [TOKEN_MINUS_EQUAL]   = "MINUS_EQUAL",   [TOKEN_PLUS_EQUAL]    = "PLUS_EQUAL",
  [TOKEN_STAR]          = "STAR",          [TOKEN_SLASH]         = "SLASH",
  [TOKEN_STAR_EQUAL]    = "STAR_EQUAL",    [TOKEN_SLASH_EQUAL]   = "SLASH_EQUAL",
  [TOKEN_MOD]           = "MOD",           [TOKEN_MOD_EQUAL]     = "MOD_EQUAL",
  [TOKEN_SEMICOLON]     = "SEMICOLON",
  [TOKEN_COLON]         = "COLON",         [TOKEN_QUESTION]      = "QUESTION",
  [TOKEN_NOT_EQUAL]     = "NOT_EQUAL",     [TOKEN_NOT]           = "NOT",
  [TOKEN_EQUAL_EQUAL]   = "EQUAL_EQUAL",   [TOKEN_EQUAL]         = "EQUAL",
  [TOKEN_LESS_EQUAL]    = "LESS_EQUAL",    [TOKEN_LESS]          = "LESS",
  [TOKEN_GREATER_EQUAL] = "GREATER_EQUAL", [TOKEN_GREATER]       = "GREATER",
  [TOKEN_AND]           = "AND",           [TOKEN_OR]            = "OR",
  [TOKEN_TAG]           = "TAG",
  [TOKEN_MEMBER]        = "MEMBER",
  [TOKEN_LSE]           = "LSE",           [TOKEN_RSE]           = "RSE",
  [TOKEN_AND_BITW_EQUAL]= "AND_BITW_EQUAL",[TOKEN_OR_BITW_EQUAL] = "OR_BITW_EQUAL",
  [TOKEN_XOR_BITW_EQUAL]= "XOR_BITW_EQUAL",
  [TOKEN_AND_BITW]      = "AND_BITW",      [TOKEN_OR_BITW]       = "OR_BITW",
  [TOKEN_NOT_BITW]      = "NOT_BITW",      [TOKEN_XOR_BITW]      = "XOR_BITW",
  [TOKEN_LEFT_SHIFT]    = "LEFT_SHIFT",    [TOKEN_RIGHT_SHIFT]   = "RIGHT_SHIFT",
  // [TOKEN_LIB]           = "LIB",
};
const char* token_type_name(TokenType type) {
  const char *name = token_names[type];
  return name != NULL ? name : "UNKNOWN";
}

typedef struct {
  char *buf;
  u32 size;
  u32 pos;
  bool truncated;
} AstBuf;

static void ast_append(AstBuf *b, const char *s) {
  while (*s) {
    if (b->pos + 2 >= b->size) { b->truncated = true; return; }
    b->buf[b->pos++] = *s++;
  }
}

static void ast_appendf(AstBuf *b, const char *fmt, ...) {
  char tmp[65];
  va_list ap;
  va_start(ap, fmt);
  u32 used = (u32)vsnprintf(tmp, sizeof(tmp), fmt, ap);
  va_end(ap);
  if (used >= sizeof(tmp)) { b->truncated = true; return; }
  ast_append(b, tmp);
}

static void ast_render(AstBuf*, const AstNode*);

static void render_type(AstBuf *b, TypeBase t) {
  if (t.is_signed) ast_appendf(b, "%s ", token_type_name(TOKEN_SIGNED));
  if (t.is_unsigned) ast_appendf(b, "%s ", token_type_name(TOKEN_UNSIGNED));
  if (t.is_short) ast_appendf(b, "%s ", token_type_name(TOKEN_SHORT));
  if (t.is_long) ast_appendf(b, "%s ", token_type_name(TOKEN_LONG));
  ast_appendf(b, "%s", token_type_name(t.base));
}

static void ast_list_render(AstBuf *b, AstNode **items, i32 count) {
  for (i32 i = 0; i < count; i++) {
    ast_append(b, " ");
    ast_render(b, items[i]);
  }
}

static void ast_render(AstBuf *b, const AstNode *n) {
  if (n == NULL) { ast_append(b, "nil"); return; }
  switch (n->type) {
    case NODE_PROGRAM: 
      ast_appendf(b, "(program %d", n->as.program.count);
      ast_list_render(b, n->as.program.statements, n->as.program.count);
      ast_append(b, ")");
      break;
    case NODE_LITERAL:
      switch (n->as.literal.literalType) {
        case TOKEN_INTEGER_LITERAL: ast_appendf(b, "(lit int %d)", n->as.literal.ival); break;
        case TOKEN_DOUBLE_LITERAL:  ast_appendf(b, "(lit double %g)", n->as.literal.dval); break;
        case TOKEN_CHAR_LITERAL:    ast_appendf(b, "(lit char '%c')", n->as.literal.cval); break;
        case TOKEN_STRING_LITERAL:  ast_appendf(b, "(lit string \"%s\")", n->as.literal.sval); break;
        default:                    ast_append(b, "(lit ?)"); break;
      }
      break;
    case NODE_VARIABLE:
      ast_appendf(b, "(var %s)", n->as.variable.name);
      break;
    case NODE_ASSIGN:
      ast_appendf(b, "(assign %s ", token_type_name(n->as.assign.op));
      ast_render(b, n->as.assign.target);
      ast_append(b, " ");
      ast_render(b, n->as.assign.value);
      ast_append(b, ")");
      break;
    case NODE_BINARY:
      ast_appendf(b, "(bin %s ", token_type_name(n->as.binary.op));
      ast_render(b, n->as.binary.left);
      ast_append(b, " ");
      ast_render(b, n->as.binary.right);
      ast_append(b, ")");
      break;
    case NODE_UNARY:
      ast_appendf(b, "(unary %s", n->as.unary.postfix ? "post" : "pre");
      ast_appendf(b, " %s ", token_type_name(n->as.unary.op));
      ast_render(b, n->as.unary.right);
      ast_append(b, ")");
      break;
    case NODE_INDEX:
      ast_append(b, "(index ");
      ast_render(b, n->as.index.object);
      ast_append(b, " ");
      ast_render(b, n->as.index.index);
      ast_append(b, ")");
      break;
    case NODE_STATEMENT:
      ast_append(b, "(stmt ");
      ast_render(b, n->as.statement.expression);
      ast_append(b, ")");
      break;
    case NODE_RETURN:
      ast_append(b, "(return");
      if (n->as.return_stmt.value) { ast_append(b, " "); ast_render(b, n->as.return_stmt.value); }
      ast_append(b, ")");
      break;
    case NODE_BREAK:    ast_append(b, "(break)"); break;
    case NODE_CONTINUE: ast_append(b, "(continue)"); break;
    case NODE_BLOCK:
      ast_appendf(b, "(block %d", n->as.block.count);
      ast_list_render(b, n->as.block.statements, n->as.block.count);
      ast_append(b, ")");
      break;
    case NODE_IF:
      ast_append(b, "(if ");
      ast_render(b, n->as.if_stmt.condition);
      ast_append(b, " ");
      ast_render(b, n->as.if_stmt.thenBranch);
      if (n->as.if_stmt.elseBranch) {
        ast_append(b, " ");
        ast_render(b, n->as.if_stmt.elseBranch);
      }
      ast_append(b, ")");
      break;
    case NODE_WHILE:
      ast_append(b, "(while ");
      ast_render(b, n->as.while_stmt.condition);
      ast_append(b, " ");
      ast_render(b, n->as.while_stmt.body);
      ast_append(b, ")");
      break;
    case NODE_DO_WHILE:
      ast_append(b, "(do-while ");
      ast_render(b, n->as.while_stmt.body);
      ast_append(b, " ");
      ast_render(b, n->as.while_stmt.condition);
      ast_append(b, ")");
      break;
    case NODE_SWITCH:
      ast_append(b, "(switch ");
      ast_render(b, n->as.switch_stmt.condition);
      ast_list_render(b, n->as.switch_stmt.cases, n->as.switch_stmt.count);
      ast_append(b, ")");
      break;
    case NODE_CASE:
      ast_append(b, n->as.case_stmt.is_default ? "(default" : "(case");
      if (n->as.case_stmt.expression) {
        ast_append(b, " ");
        ast_render(b, n->as.case_stmt.expression);
      }
      ast_list_render(b, n->as.case_stmt.statements, n->as.case_stmt.count);
      ast_append(b, ")");
      break;
    case NODE_FOR:
      ast_append(b, "(for ");
      ast_render(b, n->as.for_stmt.init);
      ast_append(b, " ");
      ast_render(b, n->as.for_stmt.condition);
      ast_append(b, " ");
      ast_render(b, n->as.for_stmt.update);
      ast_append(b, " ");
      ast_render(b, n->as.for_stmt.body);
      ast_append(b, ")");
      break;
    case NODE_DECL:
      ast_append(b, "(decl ");
      render_type(b, n->as.decl.type);
      for (i32 i = 0; i < n->as.decl.count; i++) {
        Declarator *d = &n->as.decl.declarators[i];
        ast_append(b, " (");
        for (i32 p = 0; p < d->ptr_depth; p++) ast_append(b, "*");
        if (d->ptr_depth > 0) ast_append(b, " ");
        ast_appendf(b, "%s", d->name);
        if (d->arr_rank_counts > 0) {
          for (i32 r = 0; r < d->arr_rank_counts; r++) {
            ast_append(b, "[");
            if (d->arr_dims[r]) ast_render(b, d->arr_dims[r]);
            ast_append(b, "]");
          }
        }
        if (d->init) { ast_append(b, " "); ast_render(b, d->init); }
        ast_append(b, ")");
      }
      ast_append(b, ")");
      break;
    case NODE_FUNCTION:
      ast_append(b, "(function ");
      render_type(b, n->as.function.type);
      ast_appendf(b, " %s (", n->as.function.name);
      for (i32 i = 0; i < n->as.function.param_count; i++) {
        ast_render(b, n->as.function.params[i]);
        if (i + 1 < n->as.function.param_count) ast_append(b, " ");
      }
      ast_append(b, ")");
      if (n->as.function.body) {
        ast_append(b, " ");
        ast_render(b, n->as.function.body);
      }
      ast_append(b, ")");
      break;
    case NODE_INIT_LIST:
      ast_append(b, "{");
      for (i32 i = 0; i < n->as.init_list.count; i++) {
        ast_append(b, " ");
        ast_render(b, n->as.init_list.elements[i]);
      }
      ast_append(b, " }");
      break;
  }
}

bool ast_to_string(const AstNode *node, char *out, u32 size) {
  if (size == 0) return false;
  AstBuf b = { out, size, 0, false };
  ast_render(&b, node);
  b.buf[b.pos] = '\0';
  return !b.truncated;
}
