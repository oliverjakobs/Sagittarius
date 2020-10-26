#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>

#include <assert.h>
#include <ctype.h>

#include "tb_stretchy.h"

void buf_test()
{
	int* buffer = NULL;
	enum { N = 512 };
	for (int i = 0; i < N; ++i)
		tb_stretchy_push(buffer, i);

	assert(tb_stretchy_size(buffer) == N);

	for (int i = 0; i < tb_stretchy_size(buffer); ++i)
		assert(i == buffer[i]);

	tb_stretchy_free(buffer);
}

typedef enum
{
	TOKEN_INT = 128,
	TOKEN_NAME
} TokenType;

typedef struct
{
	TokenType type;
	union
	{
		uint64_t val;
	};
} Token;

Token token;
const char* stream;

void next_token()
{
	switch (*stream)
	{
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
	{
		uint64_t val = 0;
		while (isdigit(*stream))
		{
			val *= 10;
			val += (*stream - '0');
			stream++;
		}
		token.type = TOKEN_INT;
		token.val = val;
		break;
	}
	default:
		token.type = *stream++;
		break;
	}
}

void lex_test()
{
	char* src = "+()1234+994";
	stream = src;
	next_token();
	while (token.type)
	{
		printf("Token: %d\n", token.type);
		next_token();
	}
}

int main(int argc, char** argv)
{
	buf_test();
	lex_test();
	
	return 0;
}