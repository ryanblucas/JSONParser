/*
	util.c ~ RL
	Low-level data structures. Hashmap, array list...
*/

#include "util.h"
#include <assert.h>
#include <malloc.h>
#include <stdint.h>
#include <string.h>

#define START_RESERVE 64

struct array
{
	int count,
		reserved;
	value_t* data;
};

array_t array_create(void)
{
	array_t result = malloc(sizeof * result);
	if (result == NULL)
	{
		return NULL;
	}

	result->count = 0;
	result->reserved = START_RESERVE;
	result->data = malloc(sizeof * result->data * result->reserved);
	if (result->data == NULL)
	{
		return NULL;
	}

	return result;
}

void array_destroy(array_t array)
{
	free(array->data);
	free(array);
}

static inline bool array_reserve(array_t array, int addend)
{
	int new_count = array->reserved + addend;
	value_t* new_array = realloc(array->data, sizeof * new_array * new_count);
	if (new_array == NULL)
	{
		return false;
	}
	array->reserved = new_count;
	array->data = new_array;
	return true;
}

bool array_push(array_t array, value_t val)
{
	array->data[array->count] = val;
	array->count++;
	if (array->reserved <= array->count)
	{
		if (!array_reserve(array, array->reserved))
		{
			return false;
		}
	}
	return true;
}

void array_pop(array_t array)
{
	if (array->count > 0)
	{
		array->count--;
	}
}

bool array_add(array_t array, int i, value_t val)
{
	array->count++;
	if (array->reserved <= array->count)
	{
		if (!array_reserve(array, array->reserved))
		{
			return false;
		}
	}
	assert(i >= 0 && i < array->count);
	memmove(array->data + i + 1, array->data + i, (array->count - i) * sizeof * array->data);
	array->data[i] = val;
	return true;
}

void array_remove(array_t array, int i)
{
	assert(i >= 0 && i < array->count);
	memmove(array->data + i, array->data + i + 1, (array->count - i - 1) * sizeof * array->data);
	array->count--;
}

void array_clear(array_t array)
{
	array->count = 0;
}

value_t array_get(array_t array, int i)
{
	assert(i >= 0 && i < array->count);
	return array->data[i];
}

int array_count(array_t array)
{
	return array->count;
}

/* returns array's raw data */
const value_t* array_data(array_t array)
{
	return array->data;
}

/* This hashmap implementation doesn't use buckets for the sake of simplicity; it instead uses the hash a starting index. This is likely less performant. */
#define EXISTS(map, hash) (hashmap_find((map), (hash)) < (map)->reserved)
#define NOT_FOUND -1
typedef uintmax_t hash_t;

struct key_value_pair
{
	const char* key;
	hash_t key_hash;
	value_t value;
};

struct hashmap
{
	int cache_count,
		reserved;
	const char* curr_key;
	struct key_value_pair* data;
};

hashmap_t hashmap_create(void)
{
	hashmap_t result = malloc(sizeof * result);
	if (result == NULL)
	{
		return NULL;
	}

	result->curr_key = NULL;
	result->cache_count = 0;
	result->reserved = START_RESERVE;
	result->data = calloc(result->reserved, sizeof * result->data);
	if (result->data == NULL)
	{
		return NULL;
	}
	return result;
}

void hashmap_destroy(hashmap_t map)
{
	free(map->data);
	free(map);
}

static inline bool hashmap_reserve(hashmap_t map, int addend)
{
	int new_count = map->reserved + addend;
	struct key_value_pair* new = calloc(new_count, sizeof * map->data);
	if (new == NULL)
	{
		return false;
	}
	struct hashmap prev = *map;
	*map = (struct hashmap){ .data = new, .reserved = new_count, .cache_count = 0 };
	for (int i = 0; i < prev.reserved; i++)
	{
		if (prev.data[i].key_hash != 0 &&
			!hashmap_set(map, prev.data[i].key, prev.data[i].value))
		{
			*map = prev;
			return false;
		}
	}
	free(prev.data);
	return true;
}

static inline int hashmap_find(hashmap_t map, hash_t hash)
{
	int i;
	for (i = hash % map->reserved; i < map->reserved && map->data[i].key_hash != hash; i++);
	return i;
}

static inline int hashmap_next_free(hashmap_t map, hash_t hash)
{
	int i;
	for (i = hash % map->reserved; i < map->reserved && map->data[i].key_hash != 0; i++);
	if (i >= map->reserved)
	{
		if (hashmap_reserve(map, map->reserved))
		{
			return hashmap_next_free(map, hash);
		}

		i = NOT_FOUND;
	}
	return i;
}

static inline hash_t hashmap_djb3(const char* key)
{
	hash_t hash = 5381;
	do
	{
		hash = ((hash << 5) + hash) ^ *key;
	} while (*key++);
	return hash;
}

bool hashmap_set(hashmap_t map, const char* key, value_t val)
{
	hash_t hash = hashmap_djb3(key);
	if (EXISTS(map, hash))
	{
		map->data[hashmap_find(map, hash)].value = val;
		return true;
	}

	int i = hashmap_next_free(map, hash);
	if (i == NOT_FOUND)
	{
		return false;
	}

	map->cache_count++;
	map->data[i].key = key;
	map->data[i].key_hash = hash;
	map->data[i].value = val;
	return true;
}

void hashmap_remove(hashmap_t map, const char* key)
{
	hash_t hash = hashmap_djb3(key);
	if (!EXISTS(map, hash))
	{
		return;
	}

	map->data[hashmap_find(map, hashmap_djb3(key))].key_hash = 0;
	map->cache_count--;
}

void hashmap_clear(hashmap_t map)
{
	memset(map->data, 0, sizeof * map->data * map->reserved);
	map->cache_count = 0;
}

bool hashmap_exists(const hashmap_t map, const char* key)
{
	return EXISTS(map, hashmap_djb3(key));
}

value_t hashmap_get(hashmap_t map, const char* key)
{
	hash_t hash = hashmap_djb3(key);
	assert(EXISTS(map, hash));
	return map->data[hashmap_find(map, hash)].value;
}

int hashmap_count(const hashmap_t map)
{
	return map->cache_count;
}

bool hashmap_next_set(hashmap_t map, value_t val)
{
	if (map->curr_key == NULL)
	{
		assert(val.type == TYPE_STRING);
		map->curr_key = val.data.string;
		return true;
	}

	bool result = hashmap_set(map, map->curr_key, val);
	if (result)
	{
		map->curr_key = NULL;
	}

	return result;
}

const char* hashmap_next_key(const hashmap_t map)
{
	return map->curr_key;
}

void hashmap_iterate(hashmap_t map, void* user, hashmap_iterator func)
{
	for (int i = 0; i < map->reserved; i++)
	{
		if (map->data[i].key_hash != 0)
		{
			func(map, user, map->data[i].key, map->data[i].value);
		}
	}
}