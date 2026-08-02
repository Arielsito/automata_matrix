# automata_matrix
## To compile the project
```sh
mkdir build
cd build
cmake ..
cmake --build .
```
## To run tests (criterion)
Inside the build directory, will be a binary called 'unit_tests'.
You can run specific tests or your own tests by using the flag ```--filter [pattern]```.
```bash
./unit_tests --filter "suit/name"
```
If you want to run everythinh on the suit, just replace "name" with "*". Also you can run a bunch of suits by using a pattern in the name of the suit, and you can run the by using that pattern. For example, i want to test the lexer phase, then i just name the suits with the prefix "lexer", so i can run the command
```bash
./unit_tests --filter "lexer*/*"
```
