cmake_minimum_required(VERSION 3.10)
project(automata_matrix)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED True)
include_directories(include)

add_library(compiler_core STATIC
  src/lexer.c
  src/matrices.c
  src/debug.c
  src/parser.c
  src/arena.c
)

add_executable(main
  src/main.c
)

target_link_libraries(main compiler_core)

# unit testing
option (BUILD_TESTING "Build unit tests" OFF)

if (BUILD_TESTING)
  enable_testing()
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(CRITERION REQUIRED criterion)

  add_executable(unit_tests
    test/lexer_keyword_tests.c
    test/lexer_keyword_boundary_tests.c
    test/lexer_identifier_tests.c
  )

  target_link_libraries(unit_tests compiler_core ${CRITERION_LIBRARIES})
  target_include_directories(unit_tests PRIVATE ${CRITERION_INCLUDE_DIRS})
  target_link_directories(unit_tests PRIVATE ${CRITERION_LIBRARY_DIRS})

  add_test(NAME unit_tests COMMAND unit_tests)
endif()
