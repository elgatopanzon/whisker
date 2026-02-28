/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_hash_fnv1a
 * @created     : Saturday Feb 28, 2026 13:34:21 CST
 */

#include "whisker_hash_fnv1a.h"

uint64_t w_fnv1a_hash(const void *data, size_t length, uint64_t seed)
{
	const uint8_t *p = (const uint8_t *)data;
	uint64_t hash = FNV_1A_64_OFFSET_BASIS;

	// welcome to the jungle
	for (size_t i = 0; i < length; i++)
	{
		hash ^= p[i]; // XOR into hash
		hash *= FNV_1A_64_PRIME; // prime multiply
	}

	return hash;
}
