#include "token.h"

#include <stdio.h>
#include <assert.h>
#include <ctype.h>

size_t copy_token_type_str(char* dest, size_t dest_size, TokenType type)
{
	switch (type)
	{
	case TOKEN_EOF:		return snprintf(dest, dest_size, "end of file");
	case TOKEN_INT:		return snprintf(dest, dest_size, "integer");
	case TOKEN_FLOAT:	return snprintf(dest, dest_size, "float");
	case TOKEN_NAME:	return snprintf(dest, dest_size, "name");
	default:
		if (type <= TOKEN_LAST_CHAR && isprint(type))
			return snprintf(dest, dest_size, "%c", type);
		else
			return snprintf(dest, dest_size, "<ASCII %c>", type);
	}
}

const char* token_type_str(TokenType type)
{
	static char buf[256];
	size_t n = copy_token_type_str(buf, sizeof(buf), type);
	assert(n + 1 <= sizeof(buf));
	return buf;
}


void print_token(Token token)
{
	switch (token.type)
	{
	case TOKEN_INT:
		printf("TOKEN INT: %llu\n", token.ival);
		break;
	case TOKEN_FLOAT:
		printf("TOKEN FLOAT: %f\n", token.fval);
		break;
	case TOKEN_NAME:
		printf("TOKEN NAME: %.*s\n", (int)(token.end - token.start), token.start);
		break;
	default:
		printf("TOKEN '%c'\n", token.type);
		break;
	}
}