#include "type_rules.h"
#include <limits.h>

bool is_integer_type(SemanticType type) {
  switch (type) {
    case SEM_TYPE_CHAR:
    case SEM_TYPE_SIGNED_CHAR:
    case SEM_TYPE_UNSIGNED_CHAR:
    case SEM_TYPE_SHORT:
    case SEM_TYPE_UNSIGNED_SHORT:
    case SEM_TYPE_INT:
    case SEM_TYPE_UNSIGNED_INT:
    case SEM_TYPE_LONG:
    case SEM_TYPE_UNSIGNED_LONG:
      return true;
    default:
      return false;
  }
}

bool is_arithmetic_type(SemanticType type) {
  return is_integer_type(type) ||
    type == SEM_TYPE_FLOAT ||
    type == SEM_TYPE_DOUBLE ||
    type == SEM_TYPE_LONG_DOUBLE;
}

SemanticType declared_type(TypeBase t) {
  if ((t.is_signed && t.is_unsigned) || (t.is_short && t.is_long))
    return SEM_TYPE_ERROR;
  switch (t.base) {
    case TOKEN_CHAR:
      if (t.is_short || t.is_long) return SEM_TYPE_ERROR;
      if (t.is_unsigned) return SEM_TYPE_UNSIGNED_CHAR;
      return t.is_signed ? SEM_TYPE_SIGNED_CHAR : SEM_TYPE_CHAR;
    case TOKEN_INT:
      if (t.is_short)
        return t.is_unsigned ? SEM_TYPE_UNSIGNED_SHORT : SEM_TYPE_SHORT;
      if (t.is_long)
        return t.is_unsigned ? SEM_TYPE_UNSIGNED_LONG : SEM_TYPE_LONG;
      return t.is_unsigned ? SEM_TYPE_UNSIGNED_INT : SEM_TYPE_INT;
    case TOKEN_DOUBLE:
      if (t.is_short || t.is_signed || t.is_unsigned) return SEM_TYPE_ERROR;
      return t.is_long ? SEM_TYPE_LONG_DOUBLE : SEM_TYPE_DOUBLE;
    case TOKEN_FLOAT:
    case TOKEN_VOID:
      if (t.is_short || t.is_long || t.is_signed || t.is_unsigned)
        return SEM_TYPE_ERROR;
      return t.base == TOKEN_FLOAT ? SEM_TYPE_FLOAT : SEM_TYPE_VOID;
    default:
      return SEM_TYPE_ERROR;
  }
}

SemanticType integer_promotion(SemanticType type) {
  switch (type) {
    case SEM_TYPE_SIGNED_CHAR:
    case SEM_TYPE_SHORT:
      return SEM_TYPE_INT;

    case SEM_TYPE_CHAR:
      // return CHAR_MAX <= INT_MAX
      //   ? SEM_TYPE_INT
      //   : SEM_TYPE_UNSIGNED_INT;
      //
    case SEM_TYPE_UNSIGNED_CHAR:
      return UCHAR_MAX <= INT_MAX
        ? SEM_TYPE_INT
        : SEM_TYPE_UNSIGNED_INT;

    case SEM_TYPE_UNSIGNED_SHORT:
      return USHRT_MAX <= INT_MAX
        ? SEM_TYPE_INT
        : SEM_TYPE_UNSIGNED_INT;

    default:
      return type;
  }
}

SemanticType default_argument_promotion(SemanticType type) {
  if (type == SEM_TYPE_FLOAT) return SEM_TYPE_DOUBLE;

  return integer_promotion(type);
}

SemanticType common_arithmetic_type(SemanticType left, SemanticType right) {
  if (!is_arithmetic_type(left) || !is_arithmetic_type(right)) return SEM_TYPE_ERROR;

  if (left == SEM_TYPE_LONG_DOUBLE || right == SEM_TYPE_LONG_DOUBLE) return SEM_TYPE_LONG_DOUBLE;
  if (left == SEM_TYPE_DOUBLE || right == SEM_TYPE_DOUBLE) return SEM_TYPE_DOUBLE;
  if (left == SEM_TYPE_FLOAT || right == SEM_TYPE_FLOAT) return SEM_TYPE_FLOAT;

  left = integer_promotion(left);
  right = integer_promotion(right);

  if (left == right) return left;

  if (left == SEM_TYPE_UNSIGNED_LONG || right == SEM_TYPE_UNSIGNED_LONG) return SEM_TYPE_UNSIGNED_LONG;
  if (left == SEM_TYPE_LONG || right == SEM_TYPE_LONG) {
    bool has_unsigned_int = left == SEM_TYPE_UNSIGNED_INT || right == SEM_TYPE_UNSIGNED_INT;
    if (has_unsigned_int && LONG_MAX <= UINT_MAX) return SEM_TYPE_UNSIGNED_LONG;
    return SEM_TYPE_LONG;
  }

  return SEM_TYPE_UNSIGNED_INT;
}

bool can_assign_type(SemanticType target, SemanticType value) {
  return is_arithmetic_type(target) && is_arithmetic_type(value);
}

TokenType compound_binary_operator(TokenType op) {
  switch (op) {
    case TOKEN_PLUS_EQUAL:     return TOKEN_PLUS;
    case TOKEN_MINUS_EQUAL:    return TOKEN_MINUS;
    case TOKEN_STAR_EQUAL:     return TOKEN_STAR;
    case TOKEN_SLASH_EQUAL:    return TOKEN_SLASH;
    case TOKEN_MOD_EQUAL:      return TOKEN_MOD;
    case TOKEN_AND_BITW_EQUAL: return TOKEN_AND_BITW;
    case TOKEN_OR_BITW_EQUAL:  return TOKEN_OR_BITW;
    case TOKEN_XOR_BITW_EQUAL: return TOKEN_XOR_BITW;
    case TOKEN_LSE:            return TOKEN_LEFT_SHIFT;
    case TOKEN_RSE:            return TOKEN_RIGHT_SHIFT;
    default:                   return TOKEN_ERROR;
  }
}

SemanticType binary_result_type(TokenType op, SemanticType left, SemanticType right) {
  SemanticType result = SEM_TYPE_ERROR;

  switch (op) {
    case TOKEN_PLUS:
    case TOKEN_MINUS:
    case TOKEN_STAR:
    case TOKEN_SLASH:
      result = common_arithmetic_type(left, right);
      break;

    case TOKEN_MOD:
    case TOKEN_AND_BITW:
    case TOKEN_OR_BITW:
    case TOKEN_XOR_BITW:
      if (is_integer_type(left) && is_integer_type(right))
        result = common_arithmetic_type(left, right);
      break;

    case TOKEN_LESS:
    case TOKEN_LESS_EQUAL:
    case TOKEN_GREATER:
    case TOKEN_GREATER_EQUAL:
    case TOKEN_EQUAL_EQUAL:
    case TOKEN_NOT_EQUAL:
    case TOKEN_AND:
    case TOKEN_OR:
      if (is_arithmetic_type(left) && is_arithmetic_type(right))
        result = SEM_TYPE_INT;
      break;

    case TOKEN_LEFT_SHIFT:
    case TOKEN_RIGHT_SHIFT:
      if (is_integer_type(left) && is_integer_type(right))
        result = integer_promotion(left);
      break;

    default:
      break;
  }
  return result;
}
