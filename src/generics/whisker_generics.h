/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_generics
 * @created     : Sunday Mar 02, 2025 10:56:08 CST
 */

#include "../whisker_std.h"

#ifndef WHISKER_GENERICS_H
#define WHISKER_GENERICS_H

// the type wT is used as a placeholder by the base version of a generic
// implementation
// the reason its a real type is so that the underlying non-templated code can
// be unit tested to ensure the generated generic versions reap the same
// benefits
typedef int wT;

#endif /* WHISKER_GENERICS_H */

