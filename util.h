/*
	util.h ~ RL
	Low-level data structures. Hashmap, array list...
*/

#pragma once

#include <stdbool.h>

typedef struct array* array_t;
typedef struct hashmap* hashmap_t;

typedef enum value_type
{
	TYPE_STRING,
	TYPE_NUMBER,
	TYPE_OBJECT,
	TYPE_ARRAY,
	TYPE_BOOLEAN,
	TYPE_NULL
} value_type_t;

typedef struct value
{
	value_type_t type;
	union
	{
		char* string;
		double number;
		hashmap_t object;
		array_t array;
		bool boolean;
	} data;
} value_t;

/* creates an array list w/ id */
array_t array_create_id(int id);
/* destroys an array and all its values */
void array_destroy(array_t array);
/* pushes a value onto the array */
bool array_push(array_t array, value_t val);
/* pops the top value. Use array_get to get that top value before it is popped */
void array_pop(array_t array);
/* adds value val to array at index i */
bool array_add(array_t array, int i, value_t val);
/* removes value at index i */
void array_remove(array_t array, int i);
/* clears array, sets count = 0 */
void array_clear(array_t array);
/* gets value at index i */
value_t array_get(array_t array, int i);
/* returns amount of elements in array */
int array_count(const array_t array);
/* returns array's raw data */
const value_t* array_data(const array_t array);
/* returns collections user-defined id */
int array_id(const array_t array);

/* creates an array list w/ id = -1 */
inline array_t array_create(void)
{
	return array_create_id(-1);
}

#define ARRAY_TOP(array) (array_get(array, array_count(array) - 1))

/* creates a hashmap w/ id */
hashmap_t hashmap_create_id(int id);
/* destroys a hashmap and all its entries */
void hashmap_destroy(hashmap_t map);
/* adds an entry with key and copies val into it. If the entry already exists, it replaces it. Returns false on failure, true on success */
bool hashmap_set(hashmap_t map, const char* key, value_t val);
/* destroys entry w/ key. Does nothing if map does not contain key */
void hashmap_remove(hashmap_t map, const char* key);
/* clears hashmap, sets count = 0 */
void hashmap_clear(hashmap_t map);
/* does the map contain this key */
bool hashmap_exists(const hashmap_t map, const char* key);
/* returns pointer to entry or NULL if it wasn't found */
value_t hashmap_get(hashmap_t map, const char* key);
/* returns count of entries */
int hashmap_count(const hashmap_t map);
/* returns collections user-defined id */
int hashmap_id(const hashmap_t map);

typedef void (*hashmap_iterator)(hashmap_t map, void* user, const char* key, value_t val);
/* iterates through hashmap, calling func on each valid kvp */
void hashmap_iterate(hashmap_t map, void* user, hashmap_iterator func);

/* creates an array list w/ id = -1 */
inline hashmap_t hashmap_create(void)
{
	return hashmap_create_id(-1);
}