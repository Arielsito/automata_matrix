#include "../include/matrices.h"

const State letter_dfa[][53] = {
  [0] = {
    {ACTION_NONE, .payload.c = 'a', 1}, {ACTION_NONE, .payload.c = 'b', 1}, {ACTION_NONE, .payload.c = 'c', 1},
    {ACTION_NONE, .payload.c = 'd', 1}, {ACTION_NONE, .payload.c = 'e', 1}, {ACTION_NONE, .payload.c = 'f', 1},
    {ACTION_NONE, .payload.c = 'g', 1}, {ACTION_NONE, .payload.c = 'h', 1}, {ACTION_NONE, .payload.c = 'i', 1},
    {ACTION_NONE, .payload.c = 'j', 1}, {ACTION_NONE, .payload.c = 'k', 1}, {ACTION_NONE, .payload.c = 'l', 1},
    {ACTION_NONE, .payload.c = 'm', 1}, {ACTION_NONE, .payload.c = 'n', 1}, {ACTION_NONE, .payload.c = 'o', 1},
    {ACTION_NONE, .payload.c = 'p', 1}, {ACTION_NONE, .payload.c = 'q', 1}, {ACTION_NONE, .payload.c = 'r', 1},
    {ACTION_NONE, .payload.c = 's', 1}, {ACTION_NONE, .payload.c = 't', 1}, {ACTION_NONE, .payload.c = 'u', 1}, 
    {ACTION_NONE, .payload.c = 'v', 1}, {ACTION_NONE, .payload.c = 'w', 1}, {ACTION_NONE, .payload.c = 'x', 1}, 
    {ACTION_NONE, .payload.c = 'y', 1}, {ACTION_NONE, .payload.c = 'z', 1},
    {ACTION_NONE, .payload.c = 'A', 1}, {ACTION_NONE, .payload.c = 'B', 1}, {ACTION_NONE, .payload.c = 'C', 1},
    {ACTION_NONE, .payload.c = 'D', 1}, {ACTION_NONE, .payload.c = 'E', 1}, {ACTION_NONE, .payload.c = 'F', 1},
    {ACTION_NONE, .payload.c = 'G', 1}, {ACTION_NONE, .payload.c = 'H', 1}, {ACTION_NONE, .payload.c = 'I', 1},
    {ACTION_NONE, .payload.c = 'J', 1}, {ACTION_NONE, .payload.c = 'K', 1}, {ACTION_NONE, .payload.c = 'L', 1}, 
    {ACTION_NONE, .payload.c = 'M', 1}, {ACTION_NONE, .payload.c = 'N', 1}, {ACTION_NONE, .payload.c = 'O', 1},
    {ACTION_NONE, .payload.c = 'P', 1}, {ACTION_NONE, .payload.c = 'Q', 1}, {ACTION_NONE, .payload.c = 'R', 1},
    {ACTION_NONE, .payload.c = 'S', 1}, {ACTION_NONE, .payload.c = 'T', 1}, {ACTION_NONE, .payload.c = 'U', 1}, 
    {ACTION_NONE, .payload.c = 'V', 1}, {ACTION_NONE, .payload.c = 'W', 1}, {ACTION_NONE, .payload.c = 'X', 1},
    {ACTION_NONE, .payload.c = 'Y', 1}, {ACTION_NONE, .payload.c = 'Z', 1},
    {ACTION_REJECT, .payload.token_type = TOKEN_ERROR, -1}
  },
  [1] = {
    {ACTION_ACCEPT, .payload.token_type = TOKEN_LETTER, -1},
  }
};

const State digit_dfa[][11] = {
  [0] = {
    {ACTION_NONE, .payload.c = '0', 1}, {ACTION_NONE, .payload.c = '1', 1}, {ACTION_NONE, .payload.c = '2', 1},
    {ACTION_NONE, .payload.c = '3', 1}, {ACTION_NONE, .payload.c = '4', 1}, {ACTION_NONE, .payload.c = '5', 1},
    {ACTION_NONE, .payload.c = '6', 1}, {ACTION_NONE, .payload.c = '7', 1}, {ACTION_NONE, .payload.c = '8', 1},
    {ACTION_NONE, .payload.c = '9', 1}, {ACTION_REJECT, .payload.token_type = TOKEN_ERROR, -1}
  },
  [1] = {
    {ACTION_ACCEPT, .payload.token_type = TOKEN_DIGIT, -1},
  }
};

const State identifier_dfa[][3] = {
  [0] = {
    {ACTION_CALL, .payload.sub = &letter, 1},
    {ACTION_REJECT, .payload.token_type = TOKEN_ERROR, -1}
  },
  [1] = {
    {ACTION_CALL, .payload.sub = &letter, 1},
    {ACTION_CALL, .payload.sub = &digit, 1},
    {ACTION_ACCEPT, .payload.token_type = TOKEN_IDENTIFIER, -1}
  }
};

const State number_dfa[][3] = {
  [0] = {
    {ACTION_CALL, .payload.sub = &digit, 1},
    {ACTION_NONE, .payload.c = '.', 3},
  },
  [1] = {
    {ACTION_CALL, .payload.sub = &digit, 1},
    {ACTION_NONE, .payload.c = '.', 2},
    {ACTION_ACCEPT, .payload.token_type = TOKEN_INTEGER_LITERAL, -1}
  },
  [2] = {
    {ACTION_CALL, .payload.sub = &digit, 2},
    {ACTION_ACCEPT, .payload.token_type = TOKEN_DOUBLE_LITERAL, -1},
  },
  [3] = {
    {ACTION_CALL, .payload.sub = &digit, 2},
    {ACTION_REJECT, .payload.token_type = TOKEN_ERROR, -1}
  }
};

const Dfa letter = { (State *) letter_dfa, ROWS(letter_dfa),  COLS(letter_dfa)};
const Dfa digit = { (State *) digit_dfa, ROWS(digit_dfa), COLS(digit_dfa) };
const Dfa identifier = { (State *) identifier_dfa, ROWS(identifier_dfa), COLS(identifier_dfa) };
const Dfa number = { (State *) number_dfa, ROWS(number_dfa), COLS(number_dfa) };
