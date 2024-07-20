#if 0
#include "json.h"
#include <assert.h>
#include <stdio.h>

int main()
{
#if 1 /* basic object and array test */
	{
		json_document_t obj_parse = json_parse(
			"{"
				"\"1st Str\": \"Basic string test\","
				"\"2nd Str\": \"Escape sequence \\\"Quotes here\\\", \\\\ <- single backslash.\\nNew line\\tTab\\u000ANew line w/ hex escape sequence\","
				"\"1st Num\": 12345678,"
				"\"2nd Num\": -12345678,"
				"\"3rd Num\": 1234.5678,"
				"\"4th Num\": -1234.5678,"
				"\"5th Num\": 1234.5678e4,"
				"\"6th Num\": -1234.5678e-4"
			"}");
		assert(obj_parse.error == JSON_ERROR_NONE);
		json_write_value(stdout, obj_parse.head);
	}
	printf("\n\n");
	{
		json_document_t arr_parse = json_parse(
			"["
				"\"Element 1\","
				"\"Element 2\","
				"\"Element 3\","
				"4, 5, 6"
			"]");
		assert(arr_parse.error == JSON_ERROR_NONE);
		json_write_value(stdout, arr_parse.head);
	}
	printf("\n\n");
	{
		json_document_t obj_parse = json_parse(
			"{"
				"\"Nested Object\": { "
					"\"Key\": \"Value\""
				"},"
				"\"Nested Array\": [ "
					"\"Element\""
				"]"
			"}");
		assert(obj_parse.error == JSON_ERROR_NONE);
		json_write_value(stdout, obj_parse.head);
	}
#endif
#if 0 /* json_parse_number test */
	{
		json_document_t number_parse = json_parse("12345678");
		assert(number_parse.error == JSON_ERROR_NONE);
		json_write_value(stdout, number_parse.head);
	}
	printf("\n\n");
	{
		json_document_t number_parse = json_parse("-12345678");
		assert(number_parse.error == JSON_ERROR_NONE);
		json_write_value(stdout, number_parse.head);
	}
	printf("\n\n");
	{
		json_document_t number_parse = json_parse("1234.5678");
		assert(number_parse.error == JSON_ERROR_NONE);
		json_write_value(stdout, number_parse.head);
	}
	printf("\n\n");
	{
		json_document_t number_parse = json_parse("-1234.5678");
		assert(number_parse.error == JSON_ERROR_NONE);
		json_write_value(stdout, number_parse.head);
	}
	printf("\n\n");
	{
		json_document_t number_parse = json_parse("1234.5678e4");
		assert(number_parse.error == JSON_ERROR_NONE);
		json_write_value(stdout, number_parse.head);
	}
	printf("\n\n");
	{
		json_document_t number_parse = json_parse("-1234.5678e-4");
		assert(number_parse.error == JSON_ERROR_NONE);
		json_write_value(stdout, number_parse.head);
	}
#endif
#if 0 /* json_parse_string test */
	{
		json_document_t string_parse = json_parse("\"Basic string test\"");
		assert(string_parse.error == JSON_ERROR_NONE);
		json_write_value(stdout, string_parse.head);
	}
	printf("\n\n");
	{
		json_document_t string_parse = json_parse("\"Escape sequence \\\"Quotes here\\\", \\\\ <- single backslash.\\nNew line\\tTab\\u000ANew line w/ hex escape sequence\"");
		assert(string_parse.error == JSON_ERROR_NONE);
		json_write_value(stdout, string_parse.head);
	}
	{
		json_document_t string_parse = json_parse("\"This will cause an error. \\ \"");
		assert(string_parse.error == JSON_ERROR_INVALID_ESCAPE_SEQUENCE);
	}
	{
		json_document_t string_parse = json_parse("\"So will this.\\u000G \"");
		assert(string_parse.error == JSON_ERROR_INVALID_HEX_DIGIT);
	}
#endif
#if 0 /* json_write_value test */
	value_t obj = { .type = TYPE_OBJECT, .data.object = hashmap_create() };
	{
		hashmap_set(obj.data.object, "Note", (value_t) { .type = TYPE_STRING, .data.string = "These #s assigned to fields lose their orders, that is on purpose." });
		hashmap_set(obj.data.object, "String Field 1", (value_t) { .type = TYPE_STRING, .data.string = "Value 1" });
		hashmap_set(obj.data.object, "String Field 2", (value_t) { .type = TYPE_STRING, .data.string = "Value 2" });

		value_t obj_arr = { .type = TYPE_ARRAY, .data.array = array_create() };
		hashmap_set(obj.data.object, "Array Field", obj_arr);
		{
			array_push(obj_arr.data.array, (value_t) { .type = TYPE_STRING, .data.string = "Index 0" });
			array_push(obj_arr.data.array, (value_t) { .type = TYPE_STRING, .data.string = "Index 1" });
			array_push(obj_arr.data.array, (value_t) { .type = TYPE_STRING, .data.string = "Index 2" });
			array_push(obj_arr.data.array, (value_t) { .type = TYPE_BOOLEAN, .data.boolean = true });
			array_push(obj_arr.data.array, (value_t) { .type = TYPE_STRING, .data.string = "Index 3 (boolean before me)" });
			value_t obj_arr_obj = { .type = TYPE_OBJECT, .data.object = hashmap_create() };
			array_push(obj_arr.data.array, obj_arr_obj);
			{
				hashmap_set(obj_arr_obj.data.object, "Object in Object's Array Field", (value_t) { .type = TYPE_NULL });
			}
		}
	}
	json_write_value(stdout, obj);
#endif
}
#endif