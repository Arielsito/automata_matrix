#include "common.h"
#include "arena.h"
#include "symtab.h"

static Scope* scope_new(SymTab *t, ScopeKind kind, Scope *parent) {
  Scope *s = PUSH_STRUCT(t->arena, Scope);
  if (s == NULL) return NULL;
  s->id = t->scope_count++;
  s->kind = kind;
  s->parent = parent;
  s->depth = parent ? parent->depth + 1 : 0;
  if (t->last) t->last->next = s;
  else t->first = s;
  t->last = s;
  return s;
}

static Symbol* scope_find(const Scope *s, const char *name) {
  for (Symbol *sym = s->head; sym != NULL; sym = sym->next)
    if (strcmp(sym->name, name) == 0) return sym;
  return NULL;
}

bool symtab_init(SymTab *t, Arena *arena) {
  memset(t, 0, sizeof(SymTab));
  t->arena = arena;
  t->global = scope_new(t, SCOPE_GLOBAL, NULL);
  t->current = t->global;
  return t->global != NULL;
}

Scope* symtab_enter(SymTab *t, ScopeKind kind) {
  Scope *s = scope_new(t, kind, t->current);
  if (s == NULL) return NULL;
  t->current = s;
  return s;
}

void symtab_exit(SymTab *t) {
  if (t->current->parent != NULL) t->current = t->current->parent;
}

Symbol* symtab_declare(SymTab *t, const char *name, SymbolKind kind, i32 line, Symbol **prev) {
  Symbol *found = scope_find(t->current, name);
  if (prev) *prev = found;
  if (found != NULL) return NULL;

  Symbol *s = PUSH_STRUCT(t->arena, Symbol);
  if (s == NULL) return NULL;
  s->name = name;
  s->kind = kind;
  s->line = line;
  s->scope = t->current;

  if (t->current->tail) t->current->tail->next = s;
  else t->current->head = s;
  t->current->tail = s;
  t->current->count++;
  return s;
}

Symbol* symtab_lookup_local(const SymTab *t, const char *name) {
  return scope_find(t->current, name);
}

Symbol* symtab_lookup(const SymTab *t, const char *name) {
  for (const Scope *s = t->current; s != NULL; s = s->parent) {
    Symbol *sym = scope_find(s, name);
    if (sym != NULL) return sym;
  }
  return NULL;
}

// build helpers (called from parser.c)
 
// value known at declaration: literal initializer, optionally negated
static Value const_value(AstNode *e) {
  Value v = { VAL_NONE };
  if (e == NULL) return v;
 
  if (e->type == NODE_LITERAL) {
    switch (e->as.literal.literalType) {
      case TOKEN_INTEGER_LITERAL: v.kind = VAL_INT; v.i = e->as.literal.ival; break;
      case TOKEN_DOUBLE_LITERAL: v.kind = VAL_DOUBLE; v.d = e->as.literal.dval; break;
      case TOKEN_CHAR_LITERAL: v.kind = VAL_INT; v.i = e->as.literal.cval; break;
      case TOKEN_STRING_LITERAL: v.kind = VAL_STRING; v.s = e->as.literal.sval; break;
      default: break;
    }
  } else if (e->type == NODE_UNARY && !e->as.unary.postfix && e->as.unary.op == TOKEN_MINUS) {
    v = const_value(e->as.unary.right);
    if (v.kind == VAL_INT) v.i = -v.i;
    else if (v.kind == VAL_DOUBLE) v.d = -v.d;
    else v.kind = VAL_NONE;
  }
  return v;
}
 
static void fill_function(Symbol *s, AstNode *fn) {
  s->type = fn->as.function.type;
  s->ptr_depth = fn->as.function.ret_ptr_depth;
  s->param_count = fn->as.function.param_count;
  s->is_defined = fn->as.function.body != NULL;
  s->node = fn;
}
 
// A duplicate name in the same scope is skipped (the first one wins).
bool symtab_add_decl(SymTab *t, AstNode *decl, SymbolKind kind) {
  bool ok = true;
  for (i32 i = 0; i < decl->as.decl.count; i++) {
    Declarator *d = &decl->as.decl.declarators[i];
    if (d->name == NULL) continue;
 
    Symbol *s = symtab_declare(t, d->name, kind, decl->line, NULL);
    if (s == NULL) { ok = false; continue; }
    s->type = decl->as.decl.type;
    s->ptr_depth = d->ptr_depth;
    s->arr_dims = d->arr_dims;
    s->arr_rank = d->arr_rank_counts;
    s->init = d->init;
    s->value = const_value(d->init);
    s->node = decl;
  }
  return ok;
}
 
// declares the function name in the current scope. A definition completes an earlier prototype.
bool symtab_add_function(SymTab *t, AstNode *fn) {
  if (fn->as.function.name == NULL) return false;
 
  Symbol *prev = NULL;
  Symbol *s = symtab_declare(t, fn->as.function.name, SYM_FUNC, fn->line, &prev);
  if (s != NULL) {
    fill_function(s, fn);
    return true;
  }
  if (prev != NULL && prev->kind == SYM_FUNC && !prev->is_defined && fn->as.function.body != NULL)
    fill_function(prev, fn);
  return false;
}
 
// print
 
static const char* base_name(TokenType t) {
  switch (t) {
    case TOKEN_FLOAT: return "float";
    case TOKEN_DOUBLE: return "double";
    case TOKEN_CHAR: return "char";
    case TOKEN_VOID: return "void";
    default: return "int";
  }
}
 
static const char* scope_kind_name(ScopeKind k) {
  switch (k) {
    case SCOPE_GLOBAL: return "global";
    case SCOPE_FUNCTION: return "function";
    default: return "block";
  }
}
 
static const char* symbol_kind_name(SymbolKind k) {
  switch (k) {
    case SYM_PARAM: return "param";
    case SYM_FUNC: return "func";
    default: return "var";
  }
}
 
// one row per symbol, scopes in creation order, symbols in declaration order
void symtab_print(const SymTab *t, FILE *out) {
  fprintf(out, "%-5s %-9s %-5s %-6s | %-12s %-6s %-26s %-3s %-12s %-4s %s\n",
          "SCOPE", "SCOPEKIND", "DEPTH", "PARENT", "NAME", "KIND", "TYPE", "PTR", "VALUE", "LINE", "EXTRA");
 
  for (const Scope *s = t->first; s != NULL; s = s->next) {
    for (const Symbol *sym = s->head; sym != NULL; sym = sym->next) {
      const TypeBase *ty = &sym->type;
      char type[40];
      snprintf(type, sizeof(type), "%s%s%s%s%s",
               ty->is_unsigned ? "unsigned " : "", ty->is_signed ? "signed " : "",
               ty->is_short ? "short " : "", ty->is_long ? "long " : "", base_name(ty->base));
 
      char value[24];
      switch (sym->value.kind) {
        case VAL_INT: snprintf(value, sizeof(value), "%lld", (long long)sym->value.i); break;
        case VAL_DOUBLE: snprintf(value, sizeof(value), "%g", sym->value.d); break;
        case VAL_STRING: snprintf(value, sizeof(value), "\"%.16s\"", sym->value.s); break;
        default:
          if (sym->init == NULL) snprintf(value, sizeof(value), "-");
          else if (sym->init->type == NODE_INIT_LIST) snprintf(value, sizeof(value), "{...}");
          else snprintf(value, sizeof(value), "<expr>");
      }
 
      fprintf(out, "%-5d %-9s %-5d %-6d | %-12s %-6s %-26s %-3d %-12s %-4d ",
              s->id, scope_kind_name(s->kind), s->depth, s->parent ? s->parent->id : -1,
              sym->name, symbol_kind_name(sym->kind), type, sym->ptr_depth, value, sym->line);
 
      for (i32 i = 0; i < sym->arr_rank; i++) {
        AstNode *dim = sym->arr_dims[i];
        if (dim == NULL) fprintf(out, "[]");
        else if (dim->type == NODE_LITERAL && dim->as.literal.literalType == TOKEN_INTEGER_LITERAL)
          fprintf(out, "[%d]", dim->as.literal.ival);
        else fprintf(out, "[?]");
      }
      if (sym->kind == SYM_FUNC)
        fprintf(out, "(%d params, %s)", sym->param_count, sym->is_defined ? "defined" : "prototype");
      fputc('\n', out);
    }
  }
}
