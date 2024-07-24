/*
	json.h ~ RL
*/

#pragma once
#include <stdio.h>
#include "util.h"

typedef enum json_error
{
	JSON_ERROR_NONE,
	JSON_ERROR_COMMENTS_DISABLED,
	JSON_ERROR_SYSTEM,
	JSON_ERROR_UNEXPECTED_TOKEN,
	JSON_ERROR_UNESCAPED_CONTROL_CHARACTER,
	JSON_ERROR_LEADING_ZERO,
	JSON_ERROR_INVALID_ESCAPE_SEQUENCE,
	JSON_ERROR_INVALID_HEX_DIGIT,
	JSON_ERROR_INVALID_NUMBER,
	JSON_ERROR_EXPECTED_VALUE,
	JSON_ERROR_NULL_TERMINATOR,
	JSON_ERROR_MISC,
	JSON_ERROR_COUNT,
} json_error_t;

typedef enum json_settings
{
	JSON_ALLOW_COMMENTS = 0x01,
	JSON_CHECK_BOM = 0x02,
} json_settings_t;

typedef struct json_state
{
	value_t head;
	json_error_t error;
	int pos;
	json_settings_t settings;
} json_state_t;

/*	settings are saved in a global variable instead of being passed once 
	because its likely the same settings will be used multiple times. */

/* parser's settings bitmask */
extern json_settings_t settings;

/*	parses raw given settings defined before call and returns value with any possible error/parser information.
	settings are saved at the beginning of the function to permit other threads to change settings */
json_state_t json_parse(const char* raw);
/* frees value opened by json_parse */
void json_destroy(value_t head);
/* writes value to out */
void json_write_value(FILE* out, value_t val);