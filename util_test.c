#if 0
#include <assert.h>
#include <time.h>
#include "util.h"
#include <stdlib.h>

#define TEST_COUNT 0x7FFF

int main(int argc, char* argv[])
{
	srand(time(NULL));

	hashmap_t map = hashmap_create();
	assert(map);
	char* keys = malloc(TEST_COUNT * 8);
	int* values = malloc(TEST_COUNT * sizeof * values);
	for (int i = 0; i < TEST_COUNT; i++)
	{
		assert(hashmap_count(map) == i);
		/*	random keys wouldn't work. Birthday Paradox 
			Each key is ~32.9 bits, so this isn't an issue with signed integers*/
		for (int j = 0, place = 1; j < 7; j++, place *= 26)
		{
			keys[i * 8 + j] = (i / place) % 26 + 'A';
		}

		keys[i * 8 + 7] = '\0';
		values[i] = rand();

		assert(hashmap_set(map, &keys[i * 8], (value_t) { .type = TYPE_NUMBER, .data.number = values[i] }));
	}

	for (int i = 0; i < TEST_COUNT; i++)
	{
		value_t val = hashmap_get(map, &keys[i * 8]);
		assert(val.type == TYPE_NUMBER);
		assert(val.data.number == values[i]);
		hashmap_remove(map, &keys[i * 8]);
	}

	assert(hashmap_count(map) == 0);
	hashmap_destroy(map);

	array_t arr = array_create();

	for (int i = 0; i < TEST_COUNT; i++)
	{
		assert(array_push(arr, (value_t) { .type = TYPE_NUMBER, .data.number = values[i] }));
	}

	assert(array_count(arr) == TEST_COUNT);

	for (int i = TEST_COUNT - 1; i >= 0; i--)
	{
		value_t val = array_get(arr, i);
		assert(val.type == TYPE_NUMBER);
		assert(val.data.number == values[i]);
		array_pop(arr);
	}

	assert(array_count(arr) == 0);

	for (int i = TEST_COUNT - 1; i >= 0; i--)
	{
		assert(array_add(arr, 0, (value_t) { .type = TYPE_NUMBER, .data.number = values[i] }));
	}

	assert(array_count(arr) == TEST_COUNT);

	for (int i = 0; i < TEST_COUNT; i++)
	{
		value_t val = array_get(arr, 0);
		assert(val.type == TYPE_NUMBER);
		assert(val.data.number == values[i]);
		array_remove(arr, 0);
	}

	assert(array_count(arr) == 0);

	array_destroy(arr);
}
#endif