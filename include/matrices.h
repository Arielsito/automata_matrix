#ifndef MATRICES_H
#define MATRICES_H

#include "common.h"
#include "lexer.h"

extern const State letter_dfa[][54];
extern const State digit_dfa[][11];
extern const State identifier_dfa[][3];
extern const State number_dfa[][3];
extern const State opLenght3_dfa[][3];
extern const State opLenght2_dfa[][13];
extern const State opLenght1_dfa[][14];
extern const State stringLiteral_dfa[][5];
extern const State charLiteral_dfa[][3];

extern const Dfa letter;
extern const Dfa digit;
extern const Dfa identifier;
extern const Dfa number;
extern const Dfa opLenght3;
extern const Dfa opLenght2;
extern const Dfa opLenght1;
extern const Dfa stringLiteral;
extern const Dfa charLiteral;

#endif // MATRICES_H
