/**
 * @author      : ElGatoPanzon (contact@elgatopanzon.io)
 * @file        : whisker_hashmap
 * @created     : Saturday Feb 28, 2026 19:15:53 CST
 */

#include "whisker_std.h"

#include "whisker_hashmap.h"
#include "whisker_hash_xxhash64.h"

bool w_hashmap_eq_default(const void *a, const void *b, size_t length)
{
	return memcmp(a, b, length) == 0;
}

bool w_hashmap_eq_str(const void *a, const void *b, size_t length)
{
	(void)length;
	const char *str_a = *(const char **)a;
	const char *str_b = *(const char **)b;
	return strcmp(str_a, str_b) == 0;
}

uint64_t w_hashmap_hash_str(const void *data, size_t len, uint64_t seed)
{
	(void)len;
	const char *str = *(const char **)data;
	return w_xxhash64_hash(str, strlen(str), seed);
}

void w_hashmap_init(struct w_hashmap *map, struct w_arena *arena, size_t bucket_count, size_t value_size, w_hashmap_hash_fn hash_fn, w_hashmap_equality_fn equality_fn)
{
	if (!equality_fn) { equality_fn = w_hashmap_eq_default; };

	map->buckets = w_arena_calloc(arena, sizeof(struct w_hashmap_bucket) * bucket_count);
	map->buckets_size = sizeof(struct w_hashmap_bucket) * bucket_count;
	map->buckets_length = bucket_count;
	map->total_entries = 0;
	map->value_size = value_size;
	map->hash_fn = hash_fn;
	map->equality_fn = equality_fn;
}

void w_hashmap_free(struct w_hashmap *map)
{
	for (size_t i = 0; i < map->buckets_length; ++i)
	{
		struct w_hashmap_bucket *b = &map->buckets[i];
		for (size_t j = 0; j < b->entries_length; j++)
		{
			free_null(b->entries[j].key);
			free_null(b->entries[j].value);
		}
		free_null(b->entries);
	}

	map->buckets = NULL;
	map->total_entries = 0;
}

int32_t w_hashmap_bucket_find(struct w_hashmap *map, struct w_hashmap_bucket *bucket, const void *key, size_t key_length)
{
	for (size_t i = 0; i < bucket->entries_length; i++)
	{
		if (bucket->entries[i].key_length == key_length && map->equality_fn(bucket->entries[i].key, key, key_length))
		{
			return i;
		}
	}
	return -1;
}

void w_hashmap_set(struct w_hashmap *map, const void *key, size_t key_length, const void *value)
{
	uint64_t hash = map->hash_fn(key, key_length, 0);
	size_t bucket_idx = hash & (map->buckets_length - 1);
	struct w_hashmap_bucket *bucket = &map->buckets[bucket_idx];

	// check if the key exists
	int32_t idx = w_hashmap_bucket_find(map, bucket, key, key_length);
	if (idx >= 0)
	{
		// overwrite existing value
		memcpy(bucket->entries[idx].value, value, map->value_size);
		return;
	}

	// grow bucket if needed (minimum 4 slots, then double)
	size_t new_cap = bucket->entries_length == 0 ? 4 : bucket->entries_length * 2;
	w_array_ensure_alloc(bucket->entries, new_cap);

	// append entry
	struct w_hashmap_entry *entry = &bucket->entries[bucket->entries_length];
	entry->key = w_mem_xmalloc(key_length);
	memcpy(entry->key, key, key_length);
	entry->key_length = key_length;
	entry->value = w_mem_xmalloc(map->value_size);
	memcpy(entry->value, value, map->value_size);

	bucket->entries_length++;
	map->total_entries++;
}

void *w_hashmap_get(struct w_hashmap *map, const void *key, size_t key_length)
{
	uint64_t hash = map->hash_fn(key, key_length, 0);
	size_t bucket_idx = hash & (map->buckets_length - 1);
	struct w_hashmap_bucket *bucket = &map->buckets[bucket_idx];

	// check if the key exists
	int32_t idx = w_hashmap_bucket_find(map, bucket, key, key_length);
	if (idx >= 0)
	{
		return bucket->entries[idx].value;
	}

	return NULL;
}

bool w_hashmap_remove(struct w_hashmap *map, const void *key, size_t key_length)
{
	uint64_t hash = map->hash_fn(key, key_length, 0);
	size_t bucket_idx = hash & (map->buckets_length - 1);
	struct w_hashmap_bucket *bucket = &map->buckets[bucket_idx];

	// check if the key exists
	int32_t idx = w_hashmap_bucket_find(map, bucket, key, key_length);
	if (idx < 0)
	{
		return false;
	}

	// remove value
	free_null(bucket->entries[idx].key);
	free_null(bucket->entries[idx].value);

	if ((uint32_t)idx < bucket->entries_length - 1)
	{
		bucket->entries[idx] = bucket->entries[bucket->entries_length - 1];
	}

	bucket->entries_length--;
	map->total_entries--;

	return true;
}

size_t w_hashmap_total_entries(struct w_hashmap *map)
{
	return map->total_entries;
}
