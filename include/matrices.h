#ifndef MATRICES_H
#define MATRICES_H

#include "common.h"
#include "lexer.h"

extern const State letter_dfa[][53];
extern const State number_dfa[][11];
extern const State identifier_dfa[][3];

extern const Dfa letter;
extern const Dfa number;
extern const Dfa identifier;

#endif // MATRICES_H
