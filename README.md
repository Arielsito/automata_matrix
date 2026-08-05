# automata_matrix
a compiler build from scratch in c.

## Build
```sh 
cmake -B build -S .
cmake --build build
```

the output binary is `./build/main`.

## Tests
unit tests use [Criterion](https://github.com/Snaipe/Criterion). they are disabled by default; enable with:

```sh 
cmake -B build -S . -DBUILD_TESTING=ON
cmake --build build
```

to run the while suite:
```sh 
./build/unit_tests
```
tests are grouped in suites (e.g `lexer_*`, `parser_*`). you can filter which suit run:
```sh 
./build/unit_tests --filter "parser_*/*"
./build/unit_tests --filter "parser_control/if_w_else"
```

to disable tests:
```sh 
cmake -B build -S . -DBUILD_TESTING=OFF
```

