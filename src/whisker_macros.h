/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_macros
 * @created     : Sunday Mar 30, 2025 21:29:16 CST
 * @description : string concatenation and stringification helper macros
 */

#include "whisker_std.h"

#ifndef WHISKER_MACROS_H
#define WHISKER_MACROS_H

/////////////////////
//  string macros  //
/////////////////////

// join 2 strings A and B, either literals or other macros, into a final form of
// "AB" to use as a string literal
#define MACRO_STR(A) STR(A)
#define STR(A) #A
#define JOIN_STR_NEXT(A,B) STR(A##B)
#define JOIN_STR(A, B) JOIN_STR_NEXT(A,B)

#endif // WHISKER_MACROS_H

