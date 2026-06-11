#include "../include/parser.h"
// #include "../include/debug.h"
#include <stdio.h>
#include <stdlib.h>

static void eval_expression() {
  char buff[1024];
  for (;;) {
    printf("> ");
    if (!fgets(buff, sizeof(buff), stdin)) {
      printf("\n");
      break;
    }
    bool success = compile(buff);
    if (!success) exit(65);
    else printf("Success\n");
  }
}

static char *readFile(const char* path) {
  FILE *file = fopen(path, "rb");
  if (file == NULL) {
    fprintf(stderr, "Could not open file '%s'.\n", path);
    exit(74);
  }
  fseek(file, 0L, SEEK_END);
  size_t fileSize = ftell(file);
  rewind(file);
  char *buffer = (char*)malloc(fileSize + 1);
  if (buffer == NULL) {
    fprintf(stderr, "Not enough memory to read '%s'.\n", path);
    exit(74);
  }
  size_t bytesRead = fread(buffer, sizeof(char), fileSize, file);
  if (bytesRead < fileSize) {
    fprintf(stderr, "Could not read file '%s'.\n", path);
    exit(74);
  }
  buffer[bytesRead] = '\0';
  fclose(file);
  return buffer;
}

static void runFile(const char *path) {
  char *source = readFile(path);
  bool success = compile(source);
  free(source);
  if (!success) exit(65);
  else printf("Success\n");
}

int main(i32 argc, char *argv[]) {
  if (argc == 1) eval_expression();
  else if (argc == 2) runFile(argv[1]);
  else {
    fprintf(stderr, "Error: Too many arguments.\n");
    exit(64);
  }
  return 0;
}
