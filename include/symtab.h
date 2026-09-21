#ifndef SYMTAB_H
#define SYMTAB_H

#include <stdio.h>
#include "common.h"
#include "arena.h"
#include "parser.h"

typedef enum symbolkind {
  SYM_VAR,
  SYM_PARAM,
  SYM_FUNC,
} SymbolKind;

typedef enum scopekind {
  SCOPE_GLOBAL,
  SCOPE_FUNCTION,
  SCOPE_BLOCK,
} ScopeKind;

typedef enum valuekind {
  VAL_NONE,
  VAL_INT,
  VAL_DOUBLE,
  VAL_STRING,
} ValueKind;

typedef struct Value {
  ValueKind kind;
  union {
    i64 i;
    f64 d;
    const char *s;
  };
} Value;

typedef struct Scope Scope;
typedef struct Symbol Symbol;

struct Symbol {
  const char *name;
  SymbolKind kind;
  TypeBase type;
  i32 ptr_depth;
  AstNode **arr_dims;
  i32 arr_rank;
  AstNode *init;
  Value value;
  i32 line;
  Scope *scope;
  AstNode *node;
  bool is_defined;
  i32 param_count;
  Symbol *next;
};

struct Scope {
  i32 id;               // unique per compilation, creation order
  i32 depth;            // 0 = global
  ScopeKind kind;
  Scope *parent;
  Symbol *head;
  Symbol *tail;
  i32 count;
  Scope *next;          // next scope in creation order (includes closed scopes)
};

typedef struct SymTab {
  Arena *arena;
  Scope *global;
  Scope *current;
  Scope *first;
  Scope *last;
  i32 scope_count;
} SymTab;

bool symtab_init(SymTab *, Arena *);

Scope *symtab_enter(SymTab *, ScopeKind);

void symtab_exit(SymTab *);

Symbol *symtab_declare(SymTab *, const char *, SymbolKind, i32, Symbol **prev);

Symbol *symtab_lookup(const SymTab *, const char *);
Symbol *symtab_lookup_local(const SymTab *, const char *);

bool symtab_add_decl(SymTab *, AstNode *decl, SymbolKind kind);
bool symtab_add_function(SymTab *, AstNode *fn);

void symtab_print(const SymTab *, FILE *);

#endif // SYMTAB_H
