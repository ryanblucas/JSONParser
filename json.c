/*
	json.c ~ RL
*/

#include "json.h"
#include <malloc.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "util.h"

#define START_STR_SIZE 8

json_settings_t settings = JSON_CHECK_BOM;

static inline void* json_realloc_with_zeros(const void* ptr, size_t old_size, size_t new_size)
{
	void* res = malloc(new_size);
	if (res == NULL)
	{
		return NULL;
	}

	memcpy(res, ptr, old_size);
	memset((char*)res + old_size, 0, new_size - old_size);

	return res;
}

static json_error_t json_parse_string(const char** praw, char** res)
{
	/* advance one since **praw is equal to " */
	const char* raw = (*praw) + 1;
	size_t str_size = START_STR_SIZE;
	*res = calloc(str_size, sizeof ** res);
	char* curr = *res;
	if (*res == NULL)
	{
		return JSON_ERROR_SYSTEM;
	}

	for (; *raw && *raw != '"'; raw++)
	{
		if ((int)(curr - *res) >= str_size - 2) /* allow for 2 extra bytes at the end of *res for \UXXXX escape sequences */
		{
			char* new = json_realloc_with_zeros(*res, str_size, str_size * 2);
			if (new == NULL)
			{
				free(*res);
				return JSON_ERROR_SYSTEM;
			}
			str_size *= 2;
			curr = curr - *res + new;
			*res = new;
		}

		if (*raw == '\b'
			|| *raw == '\f'
			|| *raw == '\n'
			|| *raw == '\r'
			|| *raw == '\t')
		{
			free(*res);
			return JSON_ERROR_UNESCAPED_CONTROL_CHARACTER;
		}

		if (*raw != '\\')
		{
			*curr++ = *raw;
			continue;
		}

		switch (*++raw)
		{
		case '"':
			*curr++ = '\"';
			break;
		case '\\':
			*curr++ = '\\';
			break;
		case '/':
			*curr++ = '/';
			break;
		case 'b':
			*curr++ = '\b';
			break;
		case 'f':
			*curr++ = '\f';
			break;
		case 'n':
			*curr++ = '\n';
			break;
		case 'r':
			*curr++ = '\r';
			break;
		case 't':
			*curr++ = '\t';
			break;

		case 'u':
		{
			char nibbles[4];
			for (int i = 0; *raw && i < 4; i++)
			{
				raw++;
				if (*raw >= '0' && *raw <= '9')
				{
					nibbles[i] = *raw - '0';
				}
				else if (*raw >= 'A' && *raw <= 'F')
				{
					nibbles[i] = *raw - 'A' + 0x0A;
				}
				else if (*raw >= 'a' && *raw <= 'f')
				{
					nibbles[i] = *raw - 'a' + 0x0a;
				}
				else
				{
					free(*res);
					return JSON_ERROR_INVALID_HEX_DIGIT;
				}
			}
			char first = nibbles[0] | (nibbles[1] << 4),
				second = nibbles[2] | (nibbles[3] << 4);

			if (first == 0 && second == 0)
			{
				free(*res);
				return JSON_ERROR_NULL_TERMINATOR;
			}

			if (second & 0x80)
			{
				*curr++ = first;
			}
			*curr++ = second;
			raw--;
			break;
		}
		default:
			free(*res);
			return JSON_ERROR_INVALID_ESCAPE_SEQUENCE;
		}
	}
	*praw = raw;
	return JSON_ERROR_NONE;
}

static double json_string_to_number(const char** praw)
{
	double sign = 1.0, res = 0.0;
	const char* raw = *praw;
	if (*raw == '-')
	{
		sign = -1.0;
		raw++;
	}

	char prev = 0;
	for (; *raw >= '0' && *raw <= '9'; raw++)
	{
		if (prev == '0' && *raw != '0')
		{
			return JSON_ERROR_LEADING_ZERO;
		}
		prev = *raw;
		res = res * 10 + *raw - '0';
	}

	if (sign == -1.0 && raw - 1 == *praw) /* checks against strings like "-" */
	{
		return 0.0;
	}

	*praw = raw;
	return res * sign;
}

static json_error_t json_parse_number(const char** praw, double* out, bool recursive)
{
	*out = 0.0;
	const char* raw = *praw;

	*out = json_string_to_number(&raw);
	if (*raw == '.')
	{
		raw++;
		const char* start = raw;
		*out += json_string_to_number(&raw) / pow(10.0, (double)(raw - start));
		if (start == raw)
		{
			return JSON_ERROR_UNEXPECTED_TOKEN;
		}
	}
	if (*raw == 'e' || *raw == 'E')
	{
		raw++;
		if (*raw == '+')
		{
			raw++;
		}
		const char* start = raw;
		*out *= pow(10.0, json_string_to_number(&raw));
		if (start == raw)
		{
			return JSON_ERROR_UNEXPECTED_TOKEN;
		}
	}

	*praw = raw - 1; /* back one so that we dont skip a character when json_parse increments */
	return JSON_ERROR_NONE;
}

json_state_t json_parse(const char* raw)
{
#define GUARD(condition, err) if (!(condition)) { array_destroy(stack); doc.error = err; doc.pos = (int)(raw - begin); return doc; }
	json_state_t doc = { .error = JSON_ERROR_NONE, .settings = settings };
	const char* begin = raw;
	int indent = 0;
	array_t stack = array_create();
	enum
	{
		COMMA =		0x01,
		COLON =		0x02,
		SQUIGGLY =	0x04,
		SQUARE =	0x08,
		
		NEXT_ITEM_EXPECTATION = COMMA | SQUIGGLY | SQUARE,
		DELIMITER =	COMMA | COLON | SQUIGGLY | SQUARE,

		KEY =		0x10,
		VALUE =		0x20
	} expectation = VALUE;

	GUARD(stack != NULL, JSON_ERROR_SYSTEM);

	for (; *raw; raw++)
	{
		value_t next;
		switch (*raw)
		{
		case '/':
		{
			GUARD(doc.settings & JSON_ALLOW_COMMENTS, JSON_ERROR_COMMENTS_DISABLED);
			raw++;
			if (*raw == '/')
			{
				for (; *raw && *raw != '\n'; raw++);
			}
			else if (*raw == '*')
			{
				char prev = *raw++;
				for (; *raw && !(prev == '*' && *raw == '/'); raw++);
				GUARD(*raw, JSON_ERROR_UNEXPECTED_TOKEN);
			}
			continue;
		}

		case '\"':
		{
			GUARD(expectation & (KEY | VALUE), JSON_ERROR_UNEXPECTED_TOKEN);

			char* str;
			doc.error = json_parse_string(&raw, &str);
			GUARD(doc.error == JSON_ERROR_NONE, doc.error);

			next = (value_t) { .type = TYPE_STRING, .data.string = str };

			expectation = DELIMITER;
			break;
		}

		case '{':
		{
			GUARD(expectation & VALUE, JSON_ERROR_UNEXPECTED_TOKEN);

			next = (value_t) { .type = TYPE_OBJECT, .data.object = hashmap_create() };
			GUARD(next.data.object != NULL && array_push(stack, next), JSON_ERROR_SYSTEM);

			indent++;
			expectation = KEY | SQUIGGLY;
			continue;
		}

		case '[':
		{
			GUARD(expectation & VALUE, JSON_ERROR_UNEXPECTED_TOKEN);

			next = (value_t) { .type = TYPE_ARRAY, .data.array = array_create() };
			GUARD(next.data.array != NULL && array_push(stack, next), JSON_ERROR_SYSTEM);

			indent++;
			expectation = VALUE | SQUARE;
			continue;
		}

		case 't':
		case 'f':
		case 'n':
		{
			GUARD(expectation & VALUE, JSON_ERROR_UNEXPECTED_TOKEN);
			next = (value_t) { .type = TYPE_BOOLEAN };
			if (strncmp("true", raw, 4) == 0)
			{
				next.data.boolean = true;
				raw += 3;
			}
			else if (strncmp("false", raw, 5) == 0)
			{
				next.data.boolean = false;
				raw += 4;
			}
			else if (strncmp("null", raw, 4) == 0)
			{
				next.type = TYPE_NULL;
				raw += 3;
			}
			else
			{
				GUARD(false, JSON_ERROR_UNEXPECTED_TOKEN);
			}

			expectation = NEXT_ITEM_EXPECTATION;
			break;
		}

		case ',':
		{
			GUARD(expectation & COMMA && array_count(stack) > 0, JSON_ERROR_UNEXPECTED_TOKEN);
			if (ARRAY_TOP(stack).type == TYPE_ARRAY)
			{
				expectation = VALUE;
			}
			else if (ARRAY_TOP(stack).type == TYPE_OBJECT)
			{
				expectation = KEY;
			}
			else
			{
				GUARD(false, JSON_ERROR_UNEXPECTED_TOKEN);
			}

			continue;
		}

		case ':':
		{
			GUARD(expectation & COLON && array_count(stack) > 0 && ARRAY_TOP(stack).type == TYPE_OBJECT, JSON_ERROR_UNEXPECTED_TOKEN);
			expectation = VALUE;
			continue;
		}

		case '}':
		{
			GUARD(expectation & SQUIGGLY, JSON_ERROR_UNEXPECTED_TOKEN);
			expectation = SQUARE;
		case ']':
			GUARD(expectation & SQUARE, JSON_ERROR_UNEXPECTED_TOKEN);

			if (array_count(stack) > 1)
			{
				value_t obj = ARRAY_TOP(stack), parent;
				array_pop(stack);
				parent = ARRAY_TOP(stack);
				if (parent.type == TYPE_OBJECT)
				{
					GUARD(hashmap_next_set(parent.data.object, obj), JSON_ERROR_SYSTEM);
				}
				else
				{
					GUARD(array_push(parent.data.array, obj), JSON_ERROR_SYSTEM);
				}
			}

			expectation = NEXT_ITEM_EXPECTATION;
			indent--;

			continue;
		}

		case ' ':
		case '\n':
		case '\r':
		case '\t':
			continue;

		default:
		{
			if (*raw == '-' || (*raw >= '0' && *raw <= '9'))
			{
				GUARD(expectation & VALUE, JSON_ERROR_MISC);

				next = (value_t) { .type = TYPE_NUMBER };
				doc.error = json_parse_number(&raw, &next.data.number, false);

				GUARD(doc.error == JSON_ERROR_NONE, doc.error);

				expectation = NEXT_ITEM_EXPECTATION;
			}
			else
			{
				GUARD(false, JSON_ERROR_UNEXPECTED_TOKEN);
			}
			break;
		}
		}

		if (array_count(stack) > 0)
		{
			value_t parent = ARRAY_TOP(stack);
			if (parent.type == TYPE_OBJECT)
			{
				GUARD(hashmap_next_set(parent.data.object, next), JSON_ERROR_SYSTEM);
				continue;
			}

			GUARD(array_push(parent.data.array, next), JSON_ERROR_SYSTEM);
			continue;
		}

		GUARD(array_push(stack, next), JSON_ERROR_SYSTEM);
	}

	GUARD(indent == 0, JSON_ERROR_MISC);
	GUARD(array_count(stack) == 1, JSON_ERROR_MISC);
	doc.head = ARRAY_TOP(stack);
	if (doc.head.type == TYPE_OBJECT)
	{
		GUARD(hashmap_next_key(doc.head.data.object) == NULL, JSON_ERROR_EXPECTED_VALUE);
	}
	array_destroy(stack);

	return doc;
#undef GUARD
}

static void json_destroy_map_iterator(hashmap_t map, void* user, const char* key, value_t val)
{
	json_destroy(val);
	free(key);
}

void json_destroy(value_t head)
{
	switch (head.type)
	{
	case TYPE_ARRAY:
	{
		for (int i = 0; i < array_count(head.data.array); i++)
		{
			json_destroy(array_get(head.data.array, i));
		}
		array_destroy(head.data.array);
		break;
	}
	case TYPE_OBJECT:
	{
		hashmap_iterate(head.data.object, 0, json_destroy_map_iterator);
		hashmap_destroy(head.data.object);
		break;
	}
	case TYPE_STRING:
		free(head.data.string);
		break;
	}
}

struct print_state
{
	FILE* out;
	int printed_count;
};

static int indent = 0;
#define PRINT_WITH_INDENT(frmt, ...) fprintf(out, "\n%*s" frmt, indent * 4, "", __VA_ARGS__)
static void json_print_map_iterator(hashmap_t map, void* user, const char* key, value_t val)
{
	struct print_state* state = (struct print_state*)user;

	FILE* out = state->out;
	PRINT_WITH_INDENT("\"%s\" : ", key);
	json_write_value(out, val);
	state->printed_count++;
	if (state->printed_count < hashmap_count(map))
	{
		fprintf(out, ",");
	}
}

void json_write_value(FILE* out, value_t val)
{
	switch (val.type)
	{
	case TYPE_ARRAY:
	{
		fprintf(out, "[");
		indent++;
		for (int i = 0; i < array_count(val.data.array); i++)
		{
			PRINT_WITH_INDENT("%s", ""); /* needs an argument in __VA_ARGS__ otherwise there's a syntax error */
			json_write_value(out, array_get(val.data.array, i));
			if (i + 1 < array_count(val.data.array))
			{
				fprintf(out, ",");
			}
		}
		indent--;
		if (array_count(val.data.array) > 0)
		{
			PRINT_WITH_INDENT("%s", "");
		}
		fprintf(out, "]");
		break;
	}
	case TYPE_OBJECT:
	{
		fprintf(out, "{");
		indent++;
		struct print_state this = { .out = out };
		hashmap_iterate(val.data.object, &this, json_print_map_iterator);
		indent--;
		PRINT_WITH_INDENT("%s}", "");
		break;
	}
	case TYPE_STRING:
		fprintf(out, "\"%s\"", val.data.string);
		break;
	case TYPE_NUMBER:
		fprintf(out, "%f", val.data.number);
		break;
	case TYPE_BOOLEAN:
		fprintf(out, "%s", val.data.boolean ? "true" : "false");
		break;
	case TYPE_NULL:
		fprintf(out, "null");
		break;
	}
}
#undef PRINT_WITH_INDENT