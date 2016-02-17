/*! \file macros.h
 *
 * General purpose macros used by system call template code.
 */

#ifndef __LIB_MACROS
#define __LIB_MACROS

// Get the left or right member of a tuple
#define TUPLE(X, Y) (X, Y)
#define LEFT_(X, Y) X
#define LEFT(T) LEFT_ T
#define RIGHT_(X, Y) Y
#define RIGHT(T) RIGHT_ T

// Concatenate two symbols, supporting unevaluated arguments
#define CONCAT_(A, B) A ## B
#define CONCAT(A, B) CONCAT_(A, B)

// Count the number of variadic arguments (max 3 arguments)
#define VA_NUM_ARGS(...) VA_NUM_ARGS_IMPL(__VA_ARGS__, 3, 2, 1)
#define VA_NUM_ARGS_IMPL(_1, _2, _3, N,...) N

// Map some function over the variadic arguments (max 3 arguments)
#define MAP1(F, X) F(X)
#define MAP2(F, X, Y) F(X), F(Y)
#define MAP3(F, X, Y, Z) F(X), F(Y), F(Z)
#define MAP(F, ...) CONCAT(MAP, VA_NUM_ARGS(__VA_ARGS__))(F, __VA_ARGS__)

// Transform variadic list to list of indexed tuples (max 3 arguments)
#define ENUMERATE1(X) (1, X)
#define ENUMERATE2(X, Y) (1, X), (2, Y)
#define ENUMERATE3(X, Y, Z) (1, X), (2, Y), (3, Z)
#define ENUMERATE(...) CONCAT(ENUMERATE, VA_NUM_ARGS(__VA_ARGS__))(__VA_ARGS__)

// Transform variadic argument to list of tuples with prepended element (max 3 argumentss)
#define PREPEND1(A, X) (A, X)
#define PREPEND2(A, X, Y) (A, X), (A, Y)
#define PREPEND3(A, X, Y, Z) (A, X), (A, Y), (A, Z)
#define PREPEND(A, ...) CONCAT(PREPEND, VA_NUM_ARGS(__VA_ARGS__))(A, __VA_ARGS__)

// Create an argument and access its properties
#define ARG(TYPE, NAME) TUPLE(TYPE, NAME)
#define ARG_FULL(ARG) LEFT(ARG) RIGHT(ARG)
#define ARG_TYPE(ARG) LEFT(ARG)
#define ARG_NAME(ARG) RIGHT(ARG)

// Given a tuple of the form (a, (b, c)), creates a tuple of the form (a, b, c)
#define UNCURRY3(LIST) (LEFT(LIST), LEFT(RIGHT(LIST)), RIGHT(RIGHT(LIST)))

#endif /* lib/macros.h */
