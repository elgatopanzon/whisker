/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_hashmap
 * @created     : Saturday Feb 28, 2026 15:40:03 CST
 * @description : hashmap implementation with pluggable hash funcs
 */

#include "whisker_std.h"
#include "whisker_debug.h"
#include "whisker_array.h"
#include "whisker_memory.h"
#include "whisker_arena.h"

#ifndef WHISKER_HASHMAP_H
#define WHISKER_HASHMAP_H

// hash function pointer
typedef uint64_t (*w_hashmap_hash_fn)(const void *data, size_t len, uint64_t seed);
// equality function pointer
typedef bool (*w_hashmap_equality_fn)(const void *a, const void *b, size_t len);

/* // declare hashmap entry struct */
/* #define w_hashmap_declare_entry_struct_(key_type, value_type, name) \ */
/* 	struct name##_entry { key_type key; value_type value; }; */
/*  */
/* // declare hashmap bucket struct */
/* #define w_hashmap_declare_bucket_struct_(name) \ */
/* 	struct name##_bucket { w_array_declare(struct name##_entry, entries); }; */
/*  */
/* // declare hashmap main struct */
/* #define w_hashmap_declare_struct_(name) \ */
/* 	struct name { w_array_declare(struct name##_bucket, buckets); size_t total_entries; w_hashmap_hash_fn hash_fn; w_hashmap_equality_fn equality_fn; }; */
/*  */
/* // convenience function declare all structs */
/* #define w_hashmap_declare_structs(key_type, value_type, name) \ */
/* 	w_hashmap_declare_entry_struct_(key_type, value_type, name) \ */
/* 	w_hashmap_declare_bucket_struct_(name) \ */
/* 	w_hashmap_declare_struct_(name) */
/*  */
/* // init hashmap struct with buckets using arena */
/* #define w_hashmap_init(name, bucket_capacity, arena, hash_fn_ptr, equality_fn_ptr) \ */
/* 	name.buckets = w_arena_malloc(arena, sizeof(*name.buckets) * bucket_capacity); \ */
/* 	name.buckets_size = sizeof(*name.buckets) * bucket_capacity; \ */
/* 	name.buckets_length = bucket_capacity; \ */
/* 	name.total_entries = 0; \ */
/* 	name.hash_fn = hash_fn_ptr; \ */
/* 	name.equality_fn = equality_fn_ptr;  */
/*  */
/*  */
/* // free hashmap buckets and their entries */
/* #define w_hashmap_free(name, t) \ */
/* 	for (size_t i = 0; i < name.buckets_length; i++) { \ */
/* 		if (name.buckets[i].entries) { free_null(name.buckets[i].entries); }; \ */
/* 	} \ */
/* 	name.buckets = NULL; \ */
/* 	name.total_entries = 0; */
/*  */
/* // set value in hashmap with given key */
/* #define w_hashmap_set(name, key, value) \ */
/* 	__typeof__(key) _key = (key); \ */
/* 	__typeof__(value) _value = (value); \ */
/* 	uint64_t hash = name.hash_fn(&_key, sizeof(_key), 0); \ */
/* 	size_t bucket = hash & name.buckets_length - 1; \ */
/* 	int32_t idx = w_hashmap_bucket_find(name, name.buckets[bucket], key, key_length); \ */
/*  */
/* // default equality compare function */
/* bool w_hashmap_eq_default(const void *a, const void *b, size_t length); */
/*  */
/* #include "whisker_hash_xxhash64.h"; */
/* void function_name() */
/* { */
/* 	w_hashmap_declare_structs(int, int, int_hashmap); */
/* 	struct int_hashmap map; */
/* 	struct w_arena arena; */
/* 	w_arena_init(&arena, 4096); */
/* 	w_hashmap_init(map, 65536, &arena, w_xxhash64_hash, w_hashmap_eq_default); */
/*  */
/* 	w_hashmap_set(map, 123, 456); */
/*  */
/* 	w_hashmap_free(map, int_hashmap); */
/* } */

struct w_hashmap_entry
{
	void *key;
	size_t key_length;
	void *value;
};

struct w_hashmap_bucket
{
	w_array_declare(struct w_hashmap_entry, entries);
};

struct w_hashmap
{
	w_array_declare(struct w_hashmap_bucket, buckets);
	size_t total_entries;
	size_t value_size;
	w_hashmap_hash_fn hash_fn;
	w_hashmap_equality_fn equality_fn;
};

// default equality compare function
bool w_hashmap_eq_default(const void *a, const void *b, size_t length);

// string equality compare function (dereferences char** pointers)
bool w_hashmap_eq_str(const void *a, const void *b, size_t length);

// string hash function wrapper (dereferences char** pointer, hashes string content)
uint64_t w_hashmap_hash_str(const void *data, size_t len, uint64_t seed);

// init a hashmap using an arena
void w_hashmap_init(struct w_hashmap *map, struct w_arena *arena, size_t bucket_count, size_t value_size, w_hashmap_hash_fn hash_fn, w_hashmap_equality_fn equality_fn);

// free hashmap buckets entries and values
void w_hashmap_free(struct w_hashmap *map);

// find entry in a given bucket
int32_t w_hashmap_bucket_find(struct w_hashmap *map, struct w_hashmap_bucket *bucket, const void *key, size_t key_length);

// set value in hashmap with given key
void w_hashmap_set(struct w_hashmap *map, const void *key, size_t key_length, const void *value);

// get value from hashmap with provided key
void *w_hashmap_get(struct w_hashmap *map, const void *key, size_t key_length);

// remove value from hashmap with provided key
bool w_hashmap_remove(struct w_hashmap *map, const void *key, size_t key_length);

// get hashmap total entries
size_t w_hashmap_total_entries(struct w_hashmap *map);

/*****************************
*  type-safe macro hashmap   *
*****************************/

// declare entry struct for typed hashmap
#define w_hashmap_t_entry(name) struct name##_entry

// declare bucket struct for typed hashmap
#define w_hashmap_t_bucket(name) struct name##_bucket

// declare all structs for typed hashmap
#define w_hashmap_t_declare(key_type, value_type, name) \
	w_hashmap_t_entry(name) { key_type key; value_type value; }; \
	w_hashmap_t_bucket(name) { w_array_declare(w_hashmap_t_entry(name), entries); }; \
	struct name { w_array_declare(w_hashmap_t_bucket(name), buckets); size_t total_entries; w_hashmap_hash_fn hash_fn; w_hashmap_equality_fn equality_fn; };

// init typed hashmap with arena
#define w_hashmap_t_init(map, arena, bucket_count, hash_fn_ptr, equality_fn_ptr) do { \
	(map)->buckets = w_arena_calloc((arena), sizeof(*(map)->buckets) * (bucket_count)); \
	(map)->buckets_size = sizeof(*(map)->buckets) * (bucket_count); \
	(map)->buckets_length = (bucket_count); \
	(map)->total_entries = 0; \
	(map)->hash_fn = (hash_fn_ptr); \
	(map)->equality_fn = (equality_fn_ptr) ? (equality_fn_ptr) : w_hashmap_eq_default; \
} while (0)

// free typed hashmap
#define w_hashmap_t_free(map) do { \
	for (size_t _i = 0; _i < (map)->buckets_length; _i++) { \
		if ((map)->buckets[_i].entries) { free_null((map)->buckets[_i].entries); } \
	} \
	(map)->buckets = NULL; \
	(map)->total_entries = 0; \
} while (0)

// find entry in bucket (internal)
#define w_hashmap_t_bucket_find_(map, bucket, k, result) do { \
	(result) = -1; \
	for (size_t _i = 0; _i < (bucket)->entries_length; _i++) { \
		if ((map)->equality_fn(&(bucket)->entries[_i].key, &(k), sizeof(k))) { \
			(result) = (int32_t)_i; break; \
		} \
	} \
} while (0)

// set value in typed hashmap
#define w_hashmap_t_set(map, k, v) do { \
	__typeof__((map)->buckets[0].entries[0].key) _key = (k); \
	__typeof__((map)->buckets[0].entries[0].value) _val = (v); \
	uint64_t _hash = (map)->hash_fn(&_key, sizeof(_key), 0); \
	size_t _bidx = _hash & ((map)->buckets_length - 1); \
	__typeof__((map)->buckets) _bkt = &(map)->buckets[_bidx]; \
	int32_t _idx; w_hashmap_t_bucket_find_((map), _bkt, _key, _idx); \
	if (_idx >= 0) { _bkt->entries[_idx].value = _val; } \
	else { \
		size_t _newcap = _bkt->entries_length == 0 ? 4 : _bkt->entries_length * 2; \
		w_array_ensure_alloc(_bkt->entries, _newcap); \
		_bkt->entries[_bkt->entries_length].key = _key; \
		_bkt->entries[_bkt->entries_length].value = _val; \
		_bkt->entries_length++; \
		(map)->total_entries++; \
	} \
} while (0)

// get value from typed hashmap (returns pointer or NULL)
#define w_hashmap_t_get(map, k, out_ptr) do { \
	__typeof__((map)->buckets[0].entries[0].key) _key = (k); \
	uint64_t _hash = (map)->hash_fn(&_key, sizeof(_key), 0); \
	size_t _bidx = _hash & ((map)->buckets_length - 1); \
	__typeof__((map)->buckets) _bkt = &(map)->buckets[_bidx]; \
	int32_t _idx; w_hashmap_t_bucket_find_((map), _bkt, _key, _idx); \
	(out_ptr) = (_idx >= 0) ? &_bkt->entries[_idx].value : NULL; \
} while (0)

// remove value from typed hashmap
#define w_hashmap_t_remove(map, k, removed) do { \
	__typeof__((map)->buckets[0].entries[0].key) _key = (k); \
	uint64_t _hash = (map)->hash_fn(&_key, sizeof(_key), 0); \
	size_t _bidx = _hash & ((map)->buckets_length - 1); \
	__typeof__((map)->buckets) _bkt = &(map)->buckets[_bidx]; \
	int32_t _idx; w_hashmap_t_bucket_find_((map), _bkt, _key, _idx); \
	if (_idx < 0) { (removed) = false; } \
	else { \
		if ((size_t)_idx < _bkt->entries_length - 1) { \
			_bkt->entries[_idx] = _bkt->entries[_bkt->entries_length - 1]; \
		} \
		_bkt->entries_length--; \
		(map)->total_entries--; \
		(removed) = true; \
	} \
} while (0)

// get total entries
#define w_hashmap_t_total_entries(map) ((map)->total_entries)

#endif /* WHISKER_HASHMAP_H */

