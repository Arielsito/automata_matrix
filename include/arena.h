#ifndef ARENA_H
#define ARENA_H

#include "common.h"

#define KiB(n) ((u64)(n) << 10)
#define MiB(n) ((u64)(n) << 20)
#define GiB(n) ((u64)(n) << 30)

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define ALIGN_UP_POW2(n, p) (((u64)(n) + ((u64)(p) - 1)) & (~((u64)(p) - 1)))

#define ARENA_BASE_POS (sizeof(Arena))
#define ARENA_ALIGN (sizeof(void*))

typedef struct mem_arena {
  u64 capacity;
  u64 pos;
} Arena;

Arena *arena_create(u64);
void arena_destroy(Arena*);
void *arena_push(Arena*, u64, bool);
void arena_pop(Arena*, u64);
void arena_pop_to(Arena*, u64);
void arena_clear(Arena*);

#define PUSH_STRUCT(arena, T) (T*)arena_push((arena), sizeof(T), false)
#define PUSH_STRUCT_NZ(arena, T) (T*)arena_push((arena), sizeof(T), true)
#define PUSH_ARRAY(arena, T, n) (T*)arena_push((arena), sizeof(T) * (n), false)
#define PUSH_ARRAY_NZ(arena, T, n) (T*)arena_push((arena), sizeof(T) * (n), true)

#endif // ARENA_H
