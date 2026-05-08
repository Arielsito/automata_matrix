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

const State opLenght3_dfa[][3] = {
  [0] = {
    {ACTION_NONE, .payload.c = '<', 1}, {ACTION_NONE, .payload.c = '>', 4},
    {ACTION_REJECT, .payload.token_type = TOKEN_ERROR, -1}
  },
  [1] = {
    {ACTION_NONE, .payload.c = '<', 2}, {ACTION_REJECT, .payload.token_type = TOKEN_ERROR, -1}
  },
  [2] = {
    {ACTION_NONE, .payload.c = '=', 3}, {ACTION_REJECT, .payload.token_type = TOKEN_ERROR, -1}
  },
  [3] = {
    {ACTION_ACCEPT, .payload.token_type = TOKEN_LSE, -1}
  },
  [4] = {
    {ACTION_NONE, .payload.c = '>', 5}, {ACTION_REJECT, .payload.token_type = TOKEN_ERROR, -1}
  },
  [5] = {
    {ACTION_NONE, .payload.c = '=', 6}, {ACTION_REJECT, .payload.token_type = TOKEN_ERROR, -1}
  },
  [6] = {
    {ACTION_ACCEPT, .payload.token_type = TOKEN_RSE, -1}
  }
};

const State opLenght2_dfa[][13] = {
  [0] = {
    {ACTION_NONE, .payload.c = '+', 1}, {ACTION_NONE, .payload.c = '-', 4}, {ACTION_NONE, .payload.c = '<', 8},
    {ACTION_NONE, .payload.c = '>', 11}, {ACTION_NONE, .payload.c = '=', 14}, {ACTION_NONE, .payload.c = '!', 16},
    {ACTION_NONE, .payload.c = '&', 18}, {ACTION_NONE, .payload.c = '|', 21}, {ACTION_NONE, .payload.c = '^', 24},
    {ACTION_NONE, .payload.c = '*', 26}, {ACTION_NONE, .payload.c = '/', 28}, {ACTION_NONE, .payload.c = '%', 30},
    {ACTION_REJECT, .payload.token_type = TOKEN_ERROR, -1}
  },
  [1] = {
    {ACTION_NONE, .payload.c = '+', 2}, {ACTION_NONE, .payload.c = '=', 3}, 
    {ACTION_ACCEPT, .payload.token_type = TOKEN_PLUS, -1}
  },
  [2] = {
    {ACTION_ACCEPT, .payload.token_type = TOKEN_PLUS_PLUS, -1}
  },
  [3] = {
    {ACTION_ACCEPT, .payload.token_type = TOKEN_PLUS_EQUAL, -1}
  },
  [4] = {
    {ACTION_NONE, .payload.c = '-', 5}, {ACTION_NONE, .payload.c = '=', 6}, {ACTION_NONE, .payload.c = '>', 7},
    {ACTION_ACCEPT, .payload.token_type = TOKEN_MINUS, -1}
  },
  [5] = {
    {ACTION_ACCEPT, .payload.token_type = TOKEN_MINUS_MINUS, -1}
  },
  [6] = {
    {ACTION_ACCEPT, .payload.token_type = TOKEN_MINUS_EQUAL, -1}
  },
  [7] = {
    {ACTION_ACCEPT, .payload.token_type = TOKEN_MEMBER, -1}
  },
  [8] = {
    {ACTION_NONE, .payload.c = '<', 9}, {ACTION_NONE, .payload.c = '=', 10}, 
    {ACTION_ACCEPT, .payload.token_type = TOKEN_LESS, -1}
  },
  [9] = {
    {ACTION_ACCEPT, .payload.token_type = TOKEN_LEFT_SHIFT, -1}
  },
  [10] = {
    {ACTION_ACCEPT, .payload.token_type = TOKEN_LESS_EQUAL, -1}
  },
  [11] = {
    {ACTION_NONE, .payload.c = '>', 12}, {ACTION_NONE, .payload.c = '=', 13}, 
    {ACTION_ACCEPT, .payload.token_type = TOKEN_GREATER, -1}
  },
  [12] = {
    {ACTION_ACCEPT, .payload.token_type = TOKEN_RIGHT_SHIFT, -1}
  },
  [13] = {
    {ACTION_ACCEPT, .payload.token_type = TOKEN_GREATER_EQUAL, -1}
  },
  [14] = {
    {ACTION_NONE, .payload.c = '=', 15}, {ACTION_ACCEPT, .payload.token_type = TOKEN_EQUAL, -1}
  },
  [15] = {
    {ACTION_ACCEPT, .payload.token_type = TOKEN_EQUAL_EQUAL, -1}
  },
  [16] = {
    {ACTION_NONE, .payload.c = '=', 17}, {ACTION_ACCEPT, .payload.token_type = TOKEN_NOT, -1}
  },
  [17] = {
    {ACTION_ACCEPT, .payload.token_type = TOKEN_NOT_EQUAL, -1}
  },
  [18] = {
    {ACTION_NONE, .payload.c = '&', 19}, {ACTION_NONE, .payload.c = '=', 20}, 
    {ACTION_ACCEPT, .payload.token_type = TOKEN_AND_BITW, -1}
  },
  [19] = {
    {ACTION_ACCEPT, .payload.token_type = TOKEN_AND, -1}
  },
  [20] = {
    {ACTION_ACCEPT, .payload.token_type = TOKEN_AND_BITW_EQUAL, -1}
  },
  [21] = {
    {ACTION_NONE, .payload.c = '|', 22}, {ACTION_NONE, .payload.c = '=', 23}, 
    {ACTION_ACCEPT, .payload.token_type = TOKEN_OR_BITW, -1}
  },
  [22] = {
    {ACTION_ACCEPT, .payload.token_type = TOKEN_OR, -1}
  },
  [23] = {
    {ACTION_ACCEPT, .payload.token_type = TOKEN_OR_BITW_EQUAL, -1}
  },
  [24] = {
    {ACTION_NONE, .payload.c = '=', 25}, {ACTION_ACCEPT, .payload.token_type = TOKEN_XOR_BITW, -1}
  },
  [25] = {
    {ACTION_ACCEPT, .payload.token_type = TOKEN_XOR_BITW_EQUAL, -1}
  },
  [26] = {
    {ACTION_NONE, .payload.c = '=', 27}, {ACTION_ACCEPT, .payload.token_type = TOKEN_MUL, -1}
  },
  [27] = {
    {ACTION_ACCEPT, .payload.token_type = TOKEN_MUL_EQUAL, -1}
  },
  [28] = {
    {ACTION_NONE, .payload.c = '=', 29}, {ACTION_ACCEPT, .payload.token_type = TOKEN_DIV, -1}
  },
  [29] = {
    {ACTION_ACCEPT, .payload.token_type = TOKEN_DIV_EQUAL, -1}
  },
  [30] = {
    {ACTION_NONE, .payload.c = '=', 31}, {ACTION_ACCEPT, .payload.token_type = TOKEN_MOD, -1}
  },
  [31] = {
    {ACTION_ACCEPT, .payload.token_type = TOKEN_MOD_EQUAL, -1}
  },
};

const State opLenght1_dfa[][14] = {
  [0] = {
    {ACTION_NONE, .payload.c = '~', 1}, {ACTION_NONE, .payload.c = '?', 2}, {ACTION_NONE, .payload.c = ':', 3},
    {ACTION_NONE, .payload.c = ',', 4}, {ACTION_NONE, .payload.c = ';', 5}, {ACTION_NONE, .payload.c = '.', 6},
    {ACTION_NONE, .payload.c = '(', 7}, {ACTION_NONE, .payload.c = '[', 8}, {ACTION_NONE, .payload.c = '{', 9},
    {ACTION_NONE, .payload.c = ')', 10}, {ACTION_NONE, .payload.c = ']', 11}, {ACTION_NONE, .payload.c = '}', 12},
    {ACTION_NONE, .payload.c = '#', 13},
    {ACTION_REJECT, .payload.token_type = TOKEN_ERROR, -1}
  },
  [1] = {
    {ACTION_ACCEPT, .payload.token_type = TOKEN_NOT_BITW, -1}
  },
  [2] = {
    {ACTION_ACCEPT, .payload.token_type = TOKEN_QUESTION, -1}
  },
  [3] = {
    {ACTION_ACCEPT, .payload.token_type = TOKEN_COLON, -1}
  },
  [4] = {
    {ACTION_ACCEPT, .payload.token_type = TOKEN_COMMA, -1}
  },
  [5] = {
    {ACTION_ACCEPT, .payload.token_type = TOKEN_SEMICOLON, -1}
  },
  [6] = {
    {ACTION_ACCEPT, .payload.token_type = TOKEN_DOT, -1}
  },
  [7] = {
    {ACTION_ACCEPT, .payload.token_type = TOKEN_RIGHT_PAREN, -1}
  },
  [8] = {
    {ACTION_ACCEPT, .payload.token_type = TOKEN_RIGHT_BRACKET, -1}
  },
  [9] = {
    {ACTION_ACCEPT, .payload.token_type = TOKEN_RIGHT_BRACE, -1}
  },
  [10] = {
    {ACTION_ACCEPT, .payload.token_type = TOKEN_LEFT_PAREN, -1}
  },
  [11] = {
    {ACTION_ACCEPT, .payload.token_type = TOKEN_LEFT_BRACKET, -1}
  },
  [12] = {
    {ACTION_ACCEPT, .payload.token_type = TOKEN_LEFT_BRACE, -1}
  },
  [13] = {
    {ACTION_ACCEPT, .payload.token_type = TOKEN_TAG, -1}
  }
};

const State stringLiteral_dfa[][2] = {
  [0] = {
    {ACTION_NONE, .payload.c = '"', 1}, {ACTION_REJECT, .payload.token_type = TOKEN_ERROR, -1}
  },
  [1] = {
    {ACTION_NONE, .payload.c = '"', 2}, {ACTION_ANY, .payload.c = ' ', 1}
  },
  [2] = {
    {ACTION_ACCEPT, .payload.token_type = TOKEN_STRING_LITERAL, -1}
  }
};

const State charLiteral_dfa[][2] = {
  [0] = {
    {ACTION_NONE, .payload.c = '\'', 1}, {ACTION_REJECT, .payload.token_type = TOKEN_ERROR, -1}
  },
  [1] = {
    {ACTION_NONE, .payload.c = '\\', 2}, {ACTION_ANY, .payload.c = ' ', 3}
  },
  [2] = {
    {ACTION_ANY, .payload.c = ' ', 3}
  },
  [3] = {
    {ACTION_NONE, .payload.c = '\'', 4}, {ACTION_REJECT, .payload.token_type = TOKEN_ERROR, -1}
  },
  [4] = {
    {ACTION_ACCEPT, .payload.token_type = TOKEN_CHAR_LITERAL, -1}
  }
};

const Dfa letter = { (State *) letter_dfa, ROWS(letter_dfa),  COLS(letter_dfa)};
const Dfa digit = { (State *) digit_dfa, ROWS(digit_dfa), COLS(digit_dfa) };
const Dfa identifier = { (State *) identifier_dfa, ROWS(identifier_dfa), COLS(identifier_dfa) };
const Dfa number = { (State *) number_dfa, ROWS(number_dfa), COLS(number_dfa) };
const Dfa opLenght3 = { (State *) opLenght3_dfa, ROWS(opLenght3_dfa), COLS(opLenght3_dfa)};
const Dfa opLenght2 = { (State *) opLenght2_dfa, ROWS(opLenght2_dfa), COLS(opLenght2_dfa)};
const Dfa opLenght1 = { (State *) opLenght1_dfa, ROWS(opLenght1_dfa), COLS(opLenght1_dfa)};
const Dfa stringLiteral = { (State *) stringLiteral_dfa, ROWS(stringLiteral_dfa), COLS(stringLiteral_dfa)};
const Dfa charLiteral = { (State *) charLiteral_dfa, ROWS(charLiteral_dfa), COLS(charLiteral_dfa)};

