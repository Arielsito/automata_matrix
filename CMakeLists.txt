cmake_minimum_required(VERSION 3.10)

project(automata_matrix)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED True)

include_directories(include)

add_executable(main
  src/main.c
  src/lexer.c
  src/matrices.c
  src/debug.c
  src/parser.c
  src/arena.c
)
