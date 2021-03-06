/*******************************************************************************
Copyright (c) 2021, Jyri Puputti
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*******************************************************************************/
#ifndef JMP_JSON_H
#define JMP_JSON_H

#include <stdint.h>

#ifdef JMP_JSON_STATIC
#define JMP_JSON_API static
#else
#define JMP_JSON_API extern
#endif

typedef enum
{
	JMP_OK = 0,
	JMP_INVALID_PARAM,
	JMP_SYNTAX_ERROR,
	JMP_NO_ROOT_OBJECT,
	JMP_END_OF_DATA,
} JMP_RESULT;

typedef struct
{
	const char *begin;
	const char *end;
} jmp_string_range_t;

typedef enum
{
	JMP_JSON_NULL = 0,
	JMP_JSON_BOOL,
	JMP_JSON_NUMBER,
	JMP_JSON_STRING,
	JMP_JSON_ARRAY,
	JMP_JSON_OBJECT
} JMP_JSON_TYPE;

typedef struct
{
	JMP_JSON_TYPE		type;
	jmp_string_range_t	name;
	jmp_string_range_t	data;
	uint32_t		name_length;
	uint32_t		data_length;
	const char		*cursor; // Used with object & array parsing
} jmp_json_property_t;

#ifdef __cplusplus
extern "C" {
#endif

JMP_JSON_API JMP_RESULT jmp_json_parse
(const char *data, jmp_json_property_t *out);

JMP_JSON_API JMP_RESULT jmp_json_next
(jmp_json_property_t *parent, jmp_json_property_t *out);

#ifdef __cplusplus
}
#endif
#endif // JMP_JSON_H

#ifdef JMP_JSON_IMPLEMENTATION
#include <assert.h>

typedef enum
{
	JMP_JSON__TOKEN_EOF = 0,
	JMP_JSON__TOKEN_NULL = 0x100,
	JMP_JSON__TOKEN_BOOL,
	JMP_JSON__TOKEN_NUMBER,
	JMP_JSON__TOKEN_STRING,
	JMP_JSON__TOKEN_ARRAY,
	JMP_JSON__TOKEN_OBJECT,
} JMP_JSON__TOKEN_TYPES;

typedef struct
{
	uint32_t			type;
	const char			*stream;
	jmp_string_range_t	content;
} jmp_json__lexer_state_t;

JMP_JSON_API const char *jmp_json__skip_whitespace (const char *str)
{
	while (*str != 0 && (*str == ' ' || *str == '\t' || *str == '\r' ||
						 *str == '\n'))
	{
		++ str;
	}

	return (str);
}

JMP_JSON_API const char *jmp_json__skip_line (const char *str)
{
	while (*str != 0 && *str != '\n')
	{
		++ str;
	}

	if (*str == '\n')
	{
		++ str;
	}

	return (str);
}

JMP_JSON_API const char *jmp_json__skip_comment (const char *str)
{
	const char *cursor = str;
	if (*cursor == '/')
	{
		++ cursor;
		if (*cursor == '/')
		{
			str = jmp_json__skip_line (cursor);
		}

		else if (*cursor == '*')
		{
			++ cursor;
			uint32_t level = 1;
			while (*cursor != 0 && level > 0)
			{
				switch (*cursor)
				{
					case '*':
					{
						++ cursor;
						if (*cursor == '/')
						{
							-- level;
							++ cursor;
						}
					} break;

					case '/':
					{
						++ cursor;
						if (*cursor == '*')
						{
							++ level;
							++ cursor;
						}
					} break;

					default:
					{
						++ cursor;
					} break;
				}
			}

			str = cursor;
		}
	}


	return jmp_json__skip_whitespace (str);
}

JMP_JSON_API const char *jmp_json__eat_string (const char *str)
{
	if (*str == '"')
	{
		++ str;
		while (*str != 0 && *str != '"')
		{
			switch (*str)
			{
				case '\\':
				{
					++ str;
					if (*str == '"')
					{
						++ str;
					}
				} break;

				default:
				{
					++ str;
				} break;
			}
		}

		assert (*str == '"');
		++ str;
	}

	return (str);
}

JMP_JSON_API uint32_t jmp_json__is_alphabet (const char c)
{
	return ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'));
}

JMP_JSON_API uint32_t jmp_json__is_digit (const char c)
{
	return (c >= '0' && c <= '9');
}

JMP_JSON_API const char *jmp_json__eat_number (const char *str, uint32_t *error)
{	
	if (*str == '-')
	{
		++ str;
	}
	if (!jmp_json__is_digit (*str) && error != NULL) { *error = 1; }
	while (*str != 0 && jmp_json__is_digit (*str))
	{
		++ str;
	}

	if (*str == '.')
	{
		++ str;
		if (jmp_json__is_digit (*str))
		{
			while (*str != 0 && jmp_json__is_digit (*str))
			{
				++ str;
			}
		}

		else if (error != NULL) { *error = 1; }
	}

	if (*str == 'e' || *str == 'E')
	{
		++ str;
		if (*str == '+' || *str == '-')
		{
			++ str;
			if (jmp_json__is_digit (*str))
			{
				while (*str != 0 && jmp_json__is_digit (*str))
				{
					++ str;
				}
			}

			else if (error != NULL) { *error = 1; }
		}

		else if (error != NULL)
		{
			*error = 1;
		}
	}
	return (str);
}

JMP_JSON_API const char *jmp_json__eat_plain (const char *str)
{
	while (*str != 0 && jmp_json__is_alphabet (*str))
	{
		++ str;
	}

	return (str);
}

uint32_t jmp_json__strequal (const char *a, const char *b, uint32_t length)
{
	uint32_t i;
	for (i = 0; i < length; ++ i)
	{
		if (a [i] != b [i]) { return (0); }
	}

	return (1);
}

JMP_JSON_API JMP_RESULT jmp_json__find_scope (const char *data, const char open, const char close, jmp_string_range_t *out);
JMP_JSON_API JMP_RESULT jmp_json__next_token (jmp_json__lexer_state_t *state, uint32_t simple)
{
	state->stream = jmp_json__skip_whitespace (state->stream);
	state->stream = jmp_json__skip_comment (state->stream);


	state->content.begin = state->stream;
	state->content.end = state->content.begin;
	switch (*state->stream)
	{
		case '{':
		{
			if (simple)
			{
				state->type = *state->stream;
				++ state->stream;
			}

			else
			{
				jmp_string_range_t range = {0};
				if (jmp_json__find_scope (state->stream, '{', '}', &range))
				{
					return (JMP_SYNTAX_ERROR);
				}

				state->type = JMP_JSON__TOKEN_OBJECT;
				state->content.begin = range.begin;
				state->content.end = range.end;
				state->stream = range.end + 1; // Skip one as we know it is the closing symbol
			}
		} break;

		case '[':
		{
			if (simple)
			{
				state->type = *state->stream;
				++ state->stream;
			}

			else
			{
				jmp_string_range_t range = {0};
				if (jmp_json__find_scope (state->stream, '[', ']', &range))
				{
					return (JMP_SYNTAX_ERROR);
				}
				
				state->type = JMP_JSON__TOKEN_ARRAY;
				state->content.begin = range.begin;
				state->content.end = range.end;
				state->stream = range.end + 1; // Skip one as we know it is the closing symbol
			}
		} break;

		case '"':
		{
			state->type = JMP_JSON__TOKEN_STRING;
			state->content.begin += 1;
			state->stream = jmp_json__eat_string (state->stream);
			state->content.end = state->stream - 1;
		} break;

		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
		case '-':
		{
			uint32_t error = 0;
			state->type = JMP_JSON__TOKEN_NUMBER;
			state->stream = jmp_json__eat_number (state->stream, &error);
			if (error) { return (JMP_SYNTAX_ERROR); }
		} break;

		case 'A': case 'B': case 'C': case 'D': case 'E':
		case 'F': case 'G': case 'H': case 'I': case 'J':
		case 'K': case 'L': case 'M': case 'N': case 'O':
		case 'P': case 'Q': case 'R': case 'S': case 'T':
		case 'U': case 'V': case 'W': case 'X': case 'Y':
		case 'Z': case 'a': case 'b': case 'c': case 'd':
		case 'e': case 'f': case 'g': case 'h': case 'i':
		case 'j': case 'k': case 'l': case 'm': case 'n':
		case 'o': case 'p': case 'q': case 'r': case 's':
		case 't': case 'u': case 'v': case 'w': case 'x':
		case 'y': case 'z':
		{
			state->stream = jmp_json__eat_plain (state->stream);
			size_t length = (state->stream - state->content.begin);
			switch (length)
			{
				case 4:
				{
					if (jmp_json__strequal (state->content.begin, "true", 4))
					{
						state->type = JMP_JSON__TOKEN_BOOL;
					}

					else if (jmp_json__strequal (state->content.begin, "null", 4))
					{
						state->type = JMP_JSON__TOKEN_NULL;
					}

					else
					{
						return (JMP_SYNTAX_ERROR);
					}
				} break;

				case 5:
				{
					if (jmp_json__strequal (state->content.begin, "false", 5))
					{
						state->type = JMP_JSON__TOKEN_BOOL;
					}

					else
					{
						return (JMP_SYNTAX_ERROR);
					}
				} break;

				default:
				{
					return (JMP_SYNTAX_ERROR);
				} break;
			}
			
		} break;

		default:
		{
			state->type = *state->stream;
			++ state->stream;
		} break;
	}

	if (state->content.end <= state->content.begin)
	{
		state->content.end = state->stream;
	}

	return (JMP_OK);
}

JMP_JSON_API JMP_RESULT jmp_json__find_scope
(const char *data, const char open, const char close, jmp_string_range_t *out)
{
	jmp_string_range_t scope = {NULL, NULL};
	jmp_json__lexer_state_t state = {0};
	state.stream = data;

	int32_t level = 0;
	JMP_RESULT result = JMP_OK;
	while ((result = jmp_json__next_token (&state, 1)) == JMP_OK && state.type != 0 && (scope.begin == NULL || scope.end == NULL))
	{
		if (state.type == open)
		{
			if (level == 0)
			{
				scope.begin = state.content.end;
			}

			++ level;
		}

		else if (state.type == close)
		{
			assert (level > 0);
			-- level;
			if (level == 0)
			{
				scope.end = state.content.begin;
			}
		}
	}

	if (out != NULL)
	{
		*out = scope;
	}

	return (result);
}

JMP_JSON_API JMP_RESULT
jmp_json_parse (const char *data, jmp_json_property_t *out)
{
	if (data == NULL || out == NULL)
	{
		return (JMP_INVALID_PARAM);
	}

	jmp_json__lexer_state_t state;
	state.stream = data;
	JMP_RESULT result = jmp_json__next_token (&state, 0);
	if (result == JMP_OK)
	{
		if (state.type == JMP_JSON__TOKEN_OBJECT || state.type == JMP_JSON__TOKEN_ARRAY)
		{
			out->type		= state.type - JMP_JSON__TOKEN_NULL;
			out->name.begin	= NULL;
			out->name.end	= NULL;
			out->name_length= 0;

			out->data		= state.content;
			out->data_length= (state.content.end - state.content.begin);
			out->cursor		= state.content.begin;
		}

		else
		{
			result = JMP_NO_ROOT_OBJECT;
		}
	}
	return (result);
}

JMP_JSON_API JMP_RESULT
jmp_json_next (jmp_json_property_t *parent, jmp_json_property_t *out)
{
	if (parent == NULL || out == NULL)
	{
		return (JMP_INVALID_PARAM);
	}

	if (parent->type != JMP_JSON_OBJECT && parent->type != JMP_JSON_ARRAY)
	{
		return (JMP_INVALID_PARAM);
	}

	if (parent->cursor >= parent->data.end)
	{
		return (JMP_END_OF_DATA);
	}

	jmp_json__lexer_state_t state;
	state.stream		= parent->cursor;
	JMP_RESULT result	= jmp_json__next_token (&state, 0);
	if (result == JMP_OK)
	{
		if (parent->type == JMP_JSON_OBJECT)
		{
			switch (state.type)
			{
				case JMP_JSON__TOKEN_STRING:
				{
					out->name.begin	= state.content.begin;
					out->name.end	= state.content.end;
					out->name_length= (state.content.end - state.content.begin);

					result = jmp_json__next_token (&state, 0);
					assert (result == JMP_OK && state.type == ':');

					result = jmp_json__next_token (&state, 0);
					assert (result == JMP_OK && (
							state.type == JMP_JSON__TOKEN_NULL	||
							state.type == JMP_JSON__TOKEN_BOOL	||
							state.type == JMP_JSON__TOKEN_NUMBER||
							state.type == JMP_JSON__TOKEN_STRING||
							state.type == JMP_JSON__TOKEN_OBJECT||
							state.type == JMP_JSON__TOKEN_ARRAY)
					);

					out->type		= state.type - JMP_JSON__TOKEN_NULL;
					out->data		= state.content;
					out->data_length= (state.content.end - state.content.begin);
					out->cursor = (out->type == JMP_JSON_OBJECT || out->type == JMP_JSON_ARRAY) ? state.content.begin : NULL;

					result = jmp_json__next_token (&state, 0);
					if (result == JMP_OK && (state.type != ',' && state.type != '}'))
					{
						result = JMP_SYNTAX_ERROR;
					}
				} break;

				default:
				{
					result = JMP_SYNTAX_ERROR;
				} break;
			}
		}

		else if (parent->type == JMP_JSON_ARRAY)
		{
			switch (state.type)
			{
				case JMP_JSON__TOKEN_NULL:
				case JMP_JSON__TOKEN_BOOL:
				case JMP_JSON__TOKEN_NUMBER:
				case JMP_JSON__TOKEN_STRING:
				case JMP_JSON__TOKEN_ARRAY:
				case JMP_JSON__TOKEN_OBJECT:
				{
					out->name.begin	= NULL;
					out->name.end	= NULL;
					out->name_length= 0;
					out->type		= state.type - JMP_JSON__TOKEN_NULL;
					out->data		= state.content;
					out->data_length= (state.content.end - state.content.begin);
					out->cursor		= (out->type == JMP_JSON_OBJECT || out->type == JMP_JSON_ARRAY) ? state.content.begin : NULL;

					result = jmp_json__next_token (&state, 0);
					if (result == JMP_OK && (state.type != ',' && state.type != ']'))
					{
						result = JMP_SYNTAX_ERROR;
					}
				} break;

				default:
				{
					result = JMP_SYNTAX_ERROR;
				} break;
			}
		}
	}

	parent->cursor = state.stream;
	return (result);
}

#endif // JMP_JSON_IMPLEMENTATION
