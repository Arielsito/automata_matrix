#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define CELL(dfa, state, col) (dfa)->table[(state) * (dfa)->cols + (col)]
#define COLS(matrix) (i32)(sizeof(matrix[0]) / sizeof(matrix[0][0]))
#define ROWS(matrix) (i32)(sizeof(matrix) / sizeof(matrix[0]))

typedef int8_t i8;
typedef uint8_t u8;
typedef int16_t i16;
typedef uint16_t u16;
typedef int32_t i32;
typedef uint32_t u32;
typedef int64_t i64;
typedef uint64_t u64;

typedef float f32;
typedef double f64;

#endif // COMMON_H
