#pragma once

/// \file

/// Gets the number of arguments of a variadic preprocessor macro.
/// If an empty __VA_ARGS__ is passed in, this will still return 1.
/// There is no perfect way to detect parameter lists with zero elements.
#ifndef PLASMA_VA_NUM_ARGS
#  define PLASMA_VA_NUM_ARGS(...) PLASMA_VA_NUM_ARGS_HELPER(__VA_ARGS__, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1)

#  define PLASMA_VA_NUM_ARGS_HELPER(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, N, ...) N
#endif


#define PLASMA_CALL_MACRO(macro, args) macro args


#define PLASMA_EXPAND_ARGS_1(op, a0) op(a0)
#define PLASMA_EXPAND_ARGS_2(op, a0, a1) op(a0) op(a1)
#define PLASMA_EXPAND_ARGS_3(op, a0, a1, a2) op(a0) op(a1) op(a2)
#define PLASMA_EXPAND_ARGS_4(op, a0, a1, a2, a3) op(a0) op(a1) op(a2) op(a3)
#define PLASMA_EXPAND_ARGS_5(op, a0, a1, a2, a3, a4) op(a0) op(a1) op(a2) op(a3) op(a4)
#define PLASMA_EXPAND_ARGS_6(op, a0, a1, a2, a3, a4, a5) op(a0) op(a1) op(a2) op(a3) op(a4) op(a5)
#define PLASMA_EXPAND_ARGS_7(op, a0, a1, a2, a3, a4, a5, a6) op(a0) op(a1) op(a2) op(a3) op(a4) op(a5) op(a6)
#define PLASMA_EXPAND_ARGS_8(op, a0, a1, a2, a3, a4, a5, a6, a7) op(a0) op(a1) op(a2) op(a3) op(a4) op(a5) op(a6) op(a7)
#define PLASMA_EXPAND_ARGS_9(op, a0, a1, a2, a3, a4, a5, a6, a7, a8) op(a0) op(a1) op(a2) op(a3) op(a4) op(a5) op(a6) op(a7) op(a8)
#define PLASMA_EXPAND_ARGS_10(op, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9) op(a0) op(a1) op(a2) op(a3) op(a4) op(a5) op(a6) op(a7) op(a8) op(a9)
#define PLASMA_EXPAND_ARGS_11(op, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10) \
  op(a0) op(a1) op(a2) op(a3) op(a4) op(a5) op(a6) op(a7) op(a8) op(a9) op(a10)
#define PLASMA_EXPAND_ARGS_12(op, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11) \
  op(a0) op(a1) op(a2) op(a3) op(a4) op(a5) op(a6) op(a7) op(a8) op(a9) op(a10) op(a11)

/// Variadic macro "dispatching" the arguments to the correct macro.
/// The number of arguments is found by using PLASMA_VA_NUM_ARGS(__VA_ARGS__)
#define PLASMA_EXPAND_ARGS(op, ...) PLASMA_CALL_MACRO(PLASMA_CONCAT(PLASMA_EXPAND_ARGS_, PLASMA_VA_NUM_ARGS(__VA_ARGS__)), (op, __VA_ARGS__))

//////////////////////////////////////////////////////////////////////////

#define PLASMA_EXPAND_ARGS_WITH_INDEX_1(op, a0) op(a0, 0)
#define PLASMA_EXPAND_ARGS_WITH_INDEX_2(op, a0, a1) op(a0, 0) op(a1, 1)
#define PLASMA_EXPAND_ARGS_WITH_INDEX_3(op, a0, a1, a2) op(a0, 0) op(a1, 1) op(a2, 2)
#define PLASMA_EXPAND_ARGS_WITH_INDEX_4(op, a0, a1, a2, a3) op(a0, 0) op(a1, 1) op(a2, 2) op(a3, 3)
#define PLASMA_EXPAND_ARGS_WITH_INDEX_5(op, a0, a1, a2, a3, a4) op(a0, 0) op(a1, 1) op(a2, 2) op(a3, 3) op(a4, 4)
#define PLASMA_EXPAND_ARGS_WITH_INDEX_6(op, a0, a1, a2, a3, a4, a5) op(a0, 0) op(a1, 1) op(a2, 2) op(a3, 3) op(a4, 4) op(a5, 5)
#define PLASMA_EXPAND_ARGS_WITH_INDEX_7(op, a0, a1, a2, a3, a4, a5, a6) op(a0, 0) op(a1, 1) op(a2, 2) op(a3, 3) op(a4, 4) op(a5, 5) op(a6, 6)
#define PLASMA_EXPAND_ARGS_WITH_INDEX_8(op, a0, a1, a2, a3, a4, a5, a6, a7) \
  op(a0, 0) op(a1, 1) op(a2, 2) op(a3, 3) op(a4, 4) op(a5, 5) op(a6, 6) op(a7, 7)
#define PLASMA_EXPAND_ARGS_WITH_INDEX_9(op, a0, a1, a2, a3, a4, a5, a6, a7, a8) \
  op(a0, 0) op(a1, 1) op(a2, 2) op(a3, 3) op(a4, 4) op(a5, 5) op(a6, 6) op(a7, 7) op(a8, 8)
#define PLASMA_EXPAND_ARGS_WITH_INDEX_10(op, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9) \
  op(a0, 0) op(a1, 1) op(a2, 2) op(a3, 3) op(a4, 4) op(a5, 5) op(a6, 6) op(a7, 7) op(a8, 8) op(a9, 9)

#define PLASMA_EXPAND_ARGS_WITH_INDEX(op, ...) PLASMA_CALL_MACRO(PLASMA_CONCAT(PLASMA_EXPAND_ARGS_WITH_INDEX_, PLASMA_VA_NUM_ARGS(__VA_ARGS__)), (op, __VA_ARGS__))

//////////////////////////////////////////////////////////////////////////

#define PLASMA_EXPAND_ARGS_PAIR_1(...)
#define PLASMA_EXPAND_ARGS_PAIR_2(op, a0, a1) op(a0, a1)
#define PLASMA_EXPAND_ARGS_PAIR_3(op, a0, a1, ...) op(a0, a1)
#define PLASMA_EXPAND_ARGS_PAIR_4(op, a0, a1, a2, a3) op(a0, a1) op(a2, a3)
#define PLASMA_EXPAND_ARGS_PAIR_6(op, a0, a1, a2, a3, a4, a5) op(a0, a1) op(a2, a3) op(a4, a5)
#define PLASMA_EXPAND_ARGS_PAIR_8(op, a0, a1, a2, a3, a4, a5, a6, a7) op(a0, a1) op(a2, a3) op(a4, a5) op(a6, a7)
#define PLASMA_EXPAND_ARGS_PAIR_10(op, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9) op(a0, a1) op(a2, a3) op(a4, a5) op(a6, a7) op(a8, a9)
#define PLASMA_EXPAND_ARGS_PAIR_12(op, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11) \
  op(a0, a1) op(a2, a3) op(a4, a5) op(a6, a7) op(a8, a9) op(a10, a11)
#define PLASMA_EXPAND_ARGS_PAIR_14(op, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13) \
  op(a0, a1) op(a2, a3) op(a4, a5) op(a6, a7) op(a8, a9) op(a10, a11) op(a12, a13)
#define PLASMA_EXPAND_ARGS_PAIR_16(op, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15) \
  op(a0, a1) op(a2, a3) op(a4, a5) op(a6, a7) op(a8, a9) op(a10, a11) op(a12, a13) op(a14, a15)
#define PLASMA_EXPAND_ARGS_PAIR_18(op, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17) \
  op(a0, a1) op(a2, a3) op(a4, a5) op(a6, a7) op(a8, a9) op(a10, a11) op(a12, a13) op(a14, a15) op(a16, a17)
#define PLASMA_EXPAND_ARGS_PAIR_20(op, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19) \
  op(a0, a1) op(a2, a3) op(a4, a5) op(a6, a7) op(a8, a9) op(a10, a11) op(a12, a13) op(a14, a15) op(a16, a17) op(a18, a19)

#define PLASMA_EXPAND_ARGS_PAIR(op, ...) PLASMA_CALL_MACRO(PLASMA_CONCAT(PLASMA_EXPAND_ARGS_PAIR_, PLASMA_VA_NUM_ARGS(__VA_ARGS__)), (op, __VA_ARGS__))

//////////////////////////////////////////////////////////////////////////

#define PLASMA_EXPAND_ARGS_PAIR_COMMA_1(...) /* handles the case of zero parameters (e.g. an empty __VA_ARGS__) */
#define PLASMA_EXPAND_ARGS_PAIR_COMMA_2(op, a0, a1) op(a0, a1)
#define PLASMA_EXPAND_ARGS_PAIR_COMMA_3(op, a0, a1, ...) op(a0, a1)
#define PLASMA_EXPAND_ARGS_PAIR_COMMA_4(op, a0, a1, a2, a3) op(a0, a1), op(a2, a3)
#define PLASMA_EXPAND_ARGS_PAIR_COMMA_6(op, a0, a1, a2, a3, a4, a5) op(a0, a1), op(a2, a3), op(a4, a5)
#define PLASMA_EXPAND_ARGS_PAIR_COMMA_8(op, a0, a1, a2, a3, a4, a5, a6, a7) op(a0, a1), op(a2, a3), op(a4, a5), op(a6, a7)
#define PLASMA_EXPAND_ARGS_PAIR_COMMA_10(op, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9) op(a0, a1), op(a2, a3), op(a4, a5), op(a6, a7), op(a8, a9)
#define PLASMA_EXPAND_ARGS_PAIR_COMMA_12(op, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11) \
  op(a0, a1), op(a2, a3), op(a4, a5), op(a6, a7), op(a8, a9), op(a10, a11)
#define PLASMA_EXPAND_ARGS_PAIR_COMMA_14(op, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13) \
  op(a0, a1), op(a2, a3), op(a4, a5), op(a6, a7), op(a8, a9), op(a10, a11), op(a12, a13)
#define PLASMA_EXPAND_ARGS_PAIR_COMMA_16(op, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15) \
  op(a0, a1), op(a2, a3), op(a4, a5), op(a6, a7), op(a8, a9), op(a10, a11), op(a12, a13), op(a14, a15)
#define PLASMA_EXPAND_ARGS_PAIR_COMMA_18(op, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17) \
  op(a0, a1), op(a2, a3), op(a4, a5), op(a6, a7), op(a8, a9), op(a10, a11), op(a12, a13), op(a14, a15), op(a16, a17)
#define PLASMA_EXPAND_ARGS_PAIR_COMMA_20(op, a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15, a16, a17, a18, a19) \
  op(a0, a1), op(a2, a3), op(a4, a5), op(a6, a7), op(a8, a9), op(a10, a11), op(a12, a13), op(a14, a15), op(a16, a17), op(a18, a19)

#define PLASMA_EXPAND_ARGS_PAIR_COMMA(op, ...) PLASMA_CALL_MACRO(PLASMA_CONCAT(PLASMA_EXPAND_ARGS_PAIR_COMMA_, PLASMA_VA_NUM_ARGS(__VA_ARGS__)), (op, __VA_ARGS__))

//////////////////////////////////////////////////////////////////////////

#define PLASMA_TO_BOOL_0 0
#define PLASMA_TO_BOOL_1 1
#define PLASMA_TO_BOOL_2 1
#define PLASMA_TO_BOOL_3 1
#define PLASMA_TO_BOOL_4 1
#define PLASMA_TO_BOOL_5 1
#define PLASMA_TO_BOOL_6 1
#define PLASMA_TO_BOOL_7 1
#define PLASMA_TO_BOOL_8 1
#define PLASMA_TO_BOOL_9 1

#define PLASMA_TO_BOOL(x) PLASMA_CONCAT(PLASMA_TO_BOOL_, x)

//////////////////////////////////////////////////////////////////////////

#define PLASMA_IF_0(x)
#define PLASMA_IF_1(x) x
#define PLASMA_IF(cond, x)                \
  PLASMA_CONCAT(PLASMA_IF_, PLASMA_TO_BOOL(cond)) \
  (x)

#define PLASMA_IF_ELSE_0(x, y) y
#define PLASMA_IF_ELSE_1(x, y) x
#define PLASMA_IF_ELSE(cond, x, y)             \
  PLASMA_CONCAT(PLASMA_IF_ELSE_, PLASMA_TO_BOOL(cond)) \
  (x, y)

//////////////////////////////////////////////////////////////////////////

#define PLASMA_COMMA_MARK_0
#define PLASMA_COMMA_MARK_1 ,
#define PLASMA_COMMA_IF(cond) PLASMA_CONCAT(PLASMA_COMMA_MARK_, PLASMA_TO_BOOL(cond))

//////////////////////////////////////////////////////////////////////////

#define PLASMA_LIST_0(x)
#define PLASMA_LIST_1(x) PLASMA_CONCAT(x, 0)
#define PLASMA_LIST_2(x) PLASMA_LIST_1(x), PLASMA_CONCAT(x, 1)
#define PLASMA_LIST_3(x) PLASMA_LIST_2(x), PLASMA_CONCAT(x, 2)
#define PLASMA_LIST_4(x) PLASMA_LIST_3(x), PLASMA_CONCAT(x, 3)
#define PLASMA_LIST_5(x) PLASMA_LIST_4(x), PLASMA_CONCAT(x, 4)
#define PLASMA_LIST_6(x) PLASMA_LIST_5(x), PLASMA_CONCAT(x, 5)
#define PLASMA_LIST_7(x) PLASMA_LIST_6(x), PLASMA_CONCAT(x, 6)
#define PLASMA_LIST_8(x) PLASMA_LIST_7(x), PLASMA_CONCAT(x, 7)
#define PLASMA_LIST_9(x) PLASMA_LIST_8(x), PLASMA_CONCAT(x, 8)
#define PLASMA_LIST_10(x) PLASMA_LIST_9(x), PLASMA_CONCAT(x, 9)

#define PLASMA_LIST(x, count)    \
  PLASMA_CONCAT(PLASMA_LIST_, count) \
  (x)

//////////////////////////////////////////////////////////////////////////

#define PLASMA_PAIR_LIST_0(x, y)
#define PLASMA_PAIR_LIST_1(x, y) \
  PLASMA_CONCAT(x, 0)            \
  PLASMA_CONCAT(y, 0)
#define PLASMA_PAIR_LIST_2(x, y) PLASMA_PAIR_LIST_1(x, y), PLASMA_CONCAT(x, 1) PLASMA_CONCAT(y, 1)
#define PLASMA_PAIR_LIST_3(x, y) PLASMA_PAIR_LIST_2(x, y), PLASMA_CONCAT(x, 2) PLASMA_CONCAT(y, 2)
#define PLASMA_PAIR_LIST_4(x, y) PLASMA_PAIR_LIST_3(x, y), PLASMA_CONCAT(x, 3) PLASMA_CONCAT(y, 3)
#define PLASMA_PAIR_LIST_5(x, y) PLASMA_PAIR_LIST_4(x, y), PLASMA_CONCAT(x, 4) PLASMA_CONCAT(y, 4)
#define PLASMA_PAIR_LIST_6(x, y) PLASMA_PAIR_LIST_5(x, y), PLASMA_CONCAT(x, 5) PLASMA_CONCAT(y, 5)
#define PLASMA_PAIR_LIST_7(x, y) PLASMA_PAIR_LIST_6(x, y), PLASMA_CONCAT(x, 6) PLASMA_CONCAT(y, 6)
#define PLASMA_PAIR_LIST_8(x, y) PLASMA_PAIR_LIST_7(x, y), PLASMA_CONCAT(x, 7) PLASMA_CONCAT(y, 7)
#define PLASMA_PAIR_LIST_9(x, y) PLASMA_PAIR_LIST_8(x, y), PLASMA_CONCAT(x, 8) PLASMA_CONCAT(y, 8)
#define PLASMA_PAIR_LIST_10(x, y) PLASMA_PAIR_LIST_9(x, y), PLASMA_CONCAT(x, 9) PLASMA_CONCAT(y, 9)

#define PLASMA_PAIR_LIST(x, y, count) \
  PLASMA_CONCAT(PLASMA_PAIR_LIST_, count) \
  (x, y)
