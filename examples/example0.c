#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define JMP_JSON_IMPLEMENTATION
#include "jmp_json.h"

void print_array (json_property_t *array, int32_t level);
void print_object (json_property_t *obj, int32_t level);

void print_spaces (int32_t level)
{
	for (; level > 0; -- level)
	{
		printf (" ");
	}
}

void print_array (json_property_t *array, int32_t level)
{
	JMP_RESULT result;
	json_property_t item;
	while ((result = json_next (array, &item)) == JMP_OK)
	{
		if (item.type == JMP_JSON_OBJECT)
		{
			print_object (&item, level);
		}

		else if (item.type == JMP_JSON_ARRAY)
		{
			print_array (&item, level);
		}

		else
		{
			print_spaces (level + 4);
			printf("%.*s,", item.data_length, item.data.begin);
		}
	}

	printf ("\n");
}

void print_object (json_property_t *obj, int32_t level)
{
	JMP_RESULT result;
	json_property_t prop;
	while ((result = json_next (obj, &prop)) == JMP_OK)
	{
		print_spaces (level);
		printf ("%.*s:\n", prop.name_length, prop.name.begin);
		switch (prop.type)
		{
			case JMP_JSON_OBJECT:
			{
				print_object (&prop, level + 4);
			} break;

			case JMP_JSON_ARRAY:
			{
				print_array (&prop, level + 4);
			} break;

			case JMP_JSON_NULL:
			{
				print_spaces (level + 4);
				printf ("%.*s\n", prop.data_length, prop.data.begin);
			} break;

			case JMP_JSON_BOOL:
			{
				print_spaces (level + 4);
				printf ("%.*s\n", prop.data_length, prop.data.begin);
			} break;

			case JMP_JSON_NUMBER:
			{
				print_spaces (level + 4);
				printf ("%.*s\n", prop.data_length, prop.data.begin);
			} break;

			case JMP_JSON_STRING:
			{
				print_spaces (level + 4);
				printf ("%.*s\n", prop.data_length, prop.data.begin);
			} break;
		}
	}
}

int main (int argc, char const *argv[])
{
	FILE *f = fopen ("example0.json", "r");
	fseek (f, 0, SEEK_END);
	size_t file_size = ftell (f);
	fseek (f, 0, SEEK_SET);
	char *file_data = malloc (file_size);
	memset (file_data, 0, file_size);
	fread (file_data, file_size, 1, f);
	fclose (f);

	json_property_t obj;
	JMP_RESULT result = json_parse (file_data, &obj);
	assert (result == JMP_OK);

	print_object (&obj, 0);
	return 0;
}