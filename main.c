/*
	main.c ~ RL
*/

#include <ctype.h>
#include "json.h"
#include <malloc.h>
#include <stdio.h>
#include <stdbool.h>
#include "util.h"

static hashmap_t program;

static const char* document_directory = "None";
static char* document_raw;
static json_state_t document;

struct argument_result
{
	int exit_code;
	bool cont;
};

static void argument_write(hashmap_t map, void* user, const char* key, value_t val)
{
	const char* to_write = val.data.string;
	if (to_write == NULL)
	{
		to_write = "no error.";
	}
	fprintf((FILE*)user, "\"%s\" - %s\n", key, to_write);
	free(val.data.string);
}

static struct argument_result argument_execute(const char* arg)
{
	switch (tolower(*arg))
	{
	case 'r':
	{
		arg++;
		if (!*arg)
		{
			printf("Abrupt argument ending.\n");
			return (struct argument_result) { -1 };
		}
		arg++;
		FILE* f = fopen(arg, "r");
		if (f == NULL)
		{
			printf("Failed to open file \"%s\".\n", arg);
			return (struct argument_result) { -1 };
		}
		fseek(f, 0, SEEK_END);
		long size = ftell(f);
		fseek(f, 0, SEEK_SET);
		
		char* raw = calloc((size_t)size + 1, 1);
		if (raw == NULL)
		{
			return (struct argument_result) { 1 };
		}
		fread(raw, 1, size, f);
		fclose(f);

		if (document_raw != NULL)
		{
			free(document_raw);
			json_destroy(document.head);
			printf("Unloaded previous document.\n");
		}

		document_directory = arg;
		document_raw = raw;
		document = json_parse(document_raw);

		char* err_buf = NULL;
		if (document.error != JSON_ERROR_NONE)
		{
			err_buf = malloc(80);
			if (err_buf == NULL)
			{
				printf("Failed to allocate memory.\n");
				return (struct argument_result) { -1 };
			}
			snprintf(err_buf, 80, "Error code: %i, error pos: %i -> \"%.10s\".", document.error, document.pos, document_raw + document.pos);
		}
		hashmap_set(program, document_directory, (value_t) { .type = TYPE_STRING, .data.string = err_buf });

		break;
	}

	case 'w':
	{
		arg++;
		if (!*arg)
		{
			printf("Abrupt argument ending.\n");
			return (struct argument_result) { -1 };
		}
		arg++;
		FILE* f = fopen(arg, "a");
		if (f == NULL)
		{
			printf("Failed to open file \"%s\".\n", arg);
			return (struct argument_result) { -1 };
		}

		hashmap_iterate(program, f, argument_write);
		hashmap_clear(program);

		fclose(f);
		break;
	}

	case 'p':
	{
		if (document_raw == NULL)
		{
			printf("Document never loaded.\n");
			return (struct argument_result) { -1 };
		}

		if (document.error != JSON_ERROR_NONE)
		{
			printf("Warning: Document loaded has an error, results are undefined.\n");
		}

		json_write_value(stdout, document.head);
		printf("\n");
		break;
	}

	case 'e':
	{
		if (document_raw == NULL)
		{
			printf("Document never loaded.\n");
			return (struct argument_result) { -1 };
		}

		if (document.error != JSON_ERROR_NONE)
		{
			printf("Error code: %i, error pos: %i -> \"%.10s\".\n", document.error, document.pos, document_raw + document.pos);
		}
		break;
	}

	case 'd':
		printf("Directory: %s\n", document_directory);
		break;

	default:
		printf("Unknown argument %c.\n", *arg);
		return (struct argument_result) { -1 };
	}
	return (struct argument_result) { 0, true };
}

static inline void argument_print_help(void)
{
	printf(
		"h: Prints this -- help. The help screen takes precedence over all other arguments.\n"
			"\tIn other words, passing this will effectively void all other arguments.\n"
		"r=\"[directory]\": Read file at [directory]. Quotation marks are not necessary unless if the directory has spaces; but if included, they must be double-quotes.\n"
			"\tAdditionally, this frees the previous document loaded.\n"
		"w=\"[directory]\": Appends/writes map of directories loaded thusfar in the application to their parsed documents' errors into directory.\n"
			"\tPrevious files will not be a subset of any further files. In other words, calling this writes then clears the program's state.\n"
		"p: Prints file read.\n"
		"e: Gets error code, if any.\n"
		"d: Prints directory of currently loaded file.\n"
		"Note: arguments are case-insensitive and are \"executed\" in order (except for \"h\").\n"
			"\tEx: `JSONParser p r=\"file.json\"` will result in an error since it reads the file after it's supposed to print it."
	);
}

int main(int argc, char* argv[])
{
	program = hashmap_create();

	/* help > all other arguments */
	if (argc <= 1)
	{
		argument_print_help();
		return 0;
	}
	for (int i = 1; i < argc; i++)
	{
		if (*argv[i] == 'h' || *argv[i] == 'H')
		{
			argument_print_help();
			return 0;
		}
	}

	for (int i = 1; i < argc; i++)
	{
		struct argument_result res = argument_execute(argv[i]);
		if (!res.cont)
		{
			return res.exit_code;
		}
	}

	if (document_raw != NULL)
	{
		json_destroy(document.head);
	}
}