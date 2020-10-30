#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>

#include <assert.h>
#include <ctype.h>

#include "tb_stretchy.h"


void fatal(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	printf("FATAL: ");
	vprintf(fmt, args);
	printf("\n");
	va_end(args);
	exit(1);
}

void syntax_error(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	printf("Syntax Error: ");
	vprintf(fmt, args);
	printf("\n");
	va_end(args);
}

typedef struct
{
	size_t len;
	const char* str;
} InternStr;

static InternStr* intern_table;

const char* str_intern_range(const char* start, const char* end)
{
	size_t len = end - start;
	for (InternStr* it = intern_table; it != tb_stretchy_last(intern_table); ++it)
	{
		if (it->len == len && strncmp(it->str, start, len) == 0)
			return it->str;
	}

	char* str = malloc(len + 1);
	memcpy(str, start, len);
	str[len] = '\0';
	tb_stretchy_push(intern_table, ((InternStr){ len, str }));
	return str;
}

const char* str_intern(const char* str)
{
	return str_intern_range(str, str + strlen(str));
}

void str_intern_test()
{
	char x[] = "hello";
	char y[] = "hello";

	assert(x != y);
	const char* px = str_intern(x);
	const char* py = str_intern(y);
	assert(px == py);
	char z[] = "hello!";
	const char* pz = str_intern(z);
	assert(pz != px);
}

void buf_test()
{
	int* buffer = NULL;
	assert(tb_stretchy_size(buffer) == 0);
	enum { N = 512 };
	for (int i = 0; i < N; ++i)
		tb_stretchy_push(buffer, i);

	assert(tb_stretchy_size(buffer) == N);

	for (int i = 0; i < tb_stretchy_size(buffer); ++i)
		assert(i == buffer[i]);

	tb_stretchy_free(buffer);
	assert(buffer == NULL);
	assert(tb_stretchy_size(buffer) == 0);
}

typedef enum
{
	TOKEN_INT = 128,
	TOKEN_NAME
} TokenType;

typedef struct
{
	TokenType type;
	const char* start;
	const char* end;
	union
	{
		uint64_t ival;
		const char* name;
	};
} Token;

size_t copy_token_type_str(char* dest, size_t dest_size, TokenType type)
{
	switch (type)
	{
	case '\0':			return snprintf(dest, dest_size, "end of file");
	case TOKEN_INT:		return snprintf(dest, dest_size, "integer");
	case TOKEN_NAME:	return snprintf(dest, dest_size, "name");
	default:
		if (type < 128 && isprint(type))
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

Token token;
const char* stream;

uint8_t char_to_digit[] = {
	['0'] = 0,
	['1'] = 1,
	['2'] = 2,
	['3'] = 3,
	['4'] = 4,
	['5'] = 5,
	['6'] = 6,
	['7'] = 7,
	['8'] = 8,
	['9'] = 9,
	['a'] = 10,['A'] = 10,
	['b'] = 11,['B'] = 11,
	['c'] = 12,['C'] = 12,
	['d'] = 13,['D'] = 13,
	['e'] = 14,['E'] = 14,
	['f'] = 15,['F'] = 15,
};

uint64_t scan_int()
{
	uint64_t base = 10;
	if (*stream == '0')
	{
		stream++;
		if (isdigit(*stream))
		{
			base = 8;
		}
		else
		{
			if (tolower(*stream) == 'x')
				base = 16;
			else if (tolower(*stream) == 'b')
				base = 2;
			else
				syntax_error("Invalid integer literal suffix '%c'", *stream);
			stream++;
		}
	}
	uint64_t val = 0;
	while (1)
	{
		uint64_t digit = char_to_digit[*stream];
		if (digit == 0 && *stream != '0')
			break;

		if (digit >= base)
		{
			syntax_error("Digit '%c' out of range for base %llu", *stream, base);
			digit = 0;
		}

		if (val > (UINT64_MAX - digit) / base)
		{
			syntax_error("Integer literal overflow");
			while (isdigit(*stream)) stream++;
			val = 0;
		}
		val = val * base + digit;
		stream++;
	}
	return val;
}

void next_token()
{
top:
	token.start = stream;
	switch (*stream)
	{
	case ' ': case '\n': case '\r': case '\t': case '\v':
		while (isspace(*stream)) stream++;
		goto top;
		break;
	case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
	{
		token.type = TOKEN_INT;
		token.ival = scan_int();
		break;
	}
	case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
	case 'k': case 'l': case 'm': case 'n': case 'o': case 'p': case 'q': case 'r': case 's': case 't':
	case 'u': case 'v': case 'w': case 'x': case 'y': case 'z':
	case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G': case 'H': case 'I': case 'J':
	case 'K': case 'L': case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
	case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
	case '_':
		while (isalnum(*stream) || *stream == '_') stream++;
		token.type = TOKEN_NAME;
		token.name = str_intern_range(token.start, stream);
		break;
	default:
		token.type = *stream++;
		break;
	}
	token.end = stream;
}

void init_stream(const char* str)
{
	stream = str;
	next_token();
}

void print_token(Token token)
{
	switch (token.type)
	{
	case TOKEN_INT:
		printf("TOKEN INT: %llu\n", token.ival);
		break;
	case TOKEN_NAME:
		printf("TOKEN NAME: %.*s\n", (int)(token.end - token.start), token.start);
		break;
	default:
		printf("TOKEN '%c'\n", token.type);
		break;
	}
}

inline bool is_token(TokenType type)
{
	return token.type == type;
}

inline bool is_token_name(const char* name)
{
	return token.type == TOKEN_NAME && token.name == name;
}

inline bool match_token(TokenType type)
{
	if (!is_token(type)) return false;

	next_token();
	return true;
}

inline bool expect_token(TokenType type)
{
	if (!is_token(type))
	{
		char buf[256];
		copy_token_type_str(buf, sizeof(buf), type);
		fatal("expected token %s, got %s", buf, token_type_str(token.type));
		return false;
	}

	next_token();
	return true;
}

#define assert_token(x) assert(match_token(x))
#define assert_token_name(x) assert(token.name == str_intern(x) && match_token(TOKEN_NAME))
#define assert_token_int(x) assert(token.ival = (x) && match_token(TOKEN_INT))
#define assert_token_eof() assert(match_token('\0'))

void lex_test()
{
	init_stream("0xffffffffffffffff 042 0b1111");

	assert_token_int(0xffffffffffffffffull);
	assert_token_int(042);
	assert_token_int(0xF);
	assert_token_eof();

	init_stream("XY+(XY)_HELLO1,234+Foo_34!994");
	
	assert_token_name("XY");
	assert_token('+');
	assert_token('(');
	assert_token_name("XY");
	assert_token(')');
	assert_token_name("_HELLO1");
	assert_token(',');
	assert_token_int(234);
	assert_token('+');
	assert_token_name("Foo_34");
	assert_token('!');
	assert_token_int(994);
	assert_token_eof();
}

#undef assert_token
#undef assert_token_name
#undef assert_token_int
#undef assert_token_eof

// expr3 = INT | '(' expr ')'
// expr2 = [-]expr2 | expr3
// expr1 = expr2 ([*/] expr2)*
// expr0 = expr1 ([+-] expr1)*
// expre = expr0

int parse_expr();

int parse_expr3()
{
	int val = 0;
	if (is_token(TOKEN_INT))
	{
		val = token.ival;
		next_token();
	}
	else if (match_token('('))
	{
		val = parse_expr();
		expect_token(')');
	}
	else
	{
		fatal("expted integer or '(', got %s", token_type_str(token.type));
	}
	return val;
}

int parse_expr2()
{
	if (match_token('-'))
		return -parse_expr2();
	else
		return parse_expr3();
}

int parse_expr1()
{
	int val = parse_expr2();
	while (is_token('*') || is_token('/'))
	{
		char op = token.type;
		next_token();
		int rval = parse_expr2();
		if (op == '*')
			val *= rval;
		else
		{
			assert(rval != 0);
			val /= rval;
		}
	}
	return val;
}

int parse_expr0()
{
	int val = parse_expr1();
	while (is_token('+') || is_token('-'))
	{
		char op = token.type;
		next_token();
		int rval = parse_expr1();
		if (op == '+')
			val += rval;
		else
			val -= rval;
	}
	return val;
}

int parse_expr()
{
	return parse_expr0();
}

int parse_expr_str(const char* str)
{
	init_stream(str);
	return parse_expr();
}

#define assert_expr(x) assert(parse_expr_str(#x) == (x))

void parse_test()
{
	assert_expr(1);
	assert_expr((1));
	assert_expr(1-2-3);
	assert_expr(2*3+4*5);
	assert_expr(2*(3+4)*5);
	assert_expr(2+-3);
}

#undef assert_expr

void run_tests()
{
	buf_test();
	str_intern_test();

	lex_test();

	parse_test();

}

int main(int argc, char** argv)
{
	run_tests();

	return 0;
}