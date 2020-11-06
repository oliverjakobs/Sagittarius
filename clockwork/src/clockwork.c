#include "print.h"

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

void scan_float()
{
    const char* start = stream;
    while (isdigit(*stream)) stream++;

    if (*stream == '.') stream++;

    while (isdigit(*stream)) stream++;

    if (tolower(*stream) == 'e')
    {
        stream++;
        if (*stream == '+' || *stream == '-')
            stream++;

        if (!isdigit(*stream))
            syntax_error("Expected digit after float literal exponent, found '%c'.", *stream);

        while (isdigit(*stream)) stream++;
    }

    double val = strtod(start, NULL);
    if (val == HUGE_VAL || val == -HUGE_VAL)
        syntax_error("Float literal overflow");

    token.type = TOKEN_FLOAT;
    token.fval = val;
}

void scan_int()
{
    uint64_t base = 10;
    if (*stream == '0')
    {
        stream++;
        if (tolower(*stream) == 'x')
        {
            base = 16;
            token.mod = TOKENMOD_HEX;
            stream++;
        }
        else if (tolower(*stream) == 'b')
        {
            base = 2;
            token.mod = TOKENMOD_BIN;
            stream++;
        }
        else if (isdigit(*stream))
        {
            base = 8;
            token.mod = TOKENMOD_OCT;
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
    
    token.type = TOKEN_INT;
    token.ival = val;
}

char escape_to_char[] = {
    ['n'] = '\n',
    ['r'] = '\r',
    ['t'] = '\t',
    ['v'] = '\v',
    ['b'] = '\b',
    ['a'] = '\a',
    ['0'] = 0
};

void scan_char()
{
    assert(*stream == '\'');
    stream++;

    char val = 0;
    if (*stream == '\'')
    {
        syntax_error("Char literal cannot be empty.");
        stream++;
    }
    else if (*stream == '\n')
    {
        syntax_error("Char literal cannot contain newline.");
    }
    else if (*stream == '\\')
    {
        stream++;
        val = escape_to_char[*stream];
        if (val == 0 && *stream != '0')
            syntax_error("Invalid char literal escape '\\%c'.", *stream);

        stream++;
    }
    else
    {
        val = *stream;
        stream++;
    }

    if (*stream != '\'')
        syntax_error("Expected closing char quote, got '%c'.", *stream);
    else
        stream++;

    token.type = TOKEN_INT;
    token.mod = TOKENMOD_CHAR;
    token.ival = val;
}

void scan_str()
{
    assert(*stream == '"');
    stream++;

    char* str = NULL;
    while (*stream && *stream != '"')
    {
        char val = *stream;
        if (val == '\n')
        {
            syntax_error("Char literal cannot contain newline.");
        }
        else if (val == '\\')
        {
            stream++;
            val = escape_to_char[*stream];
            if (val == 0 && *stream != '0')
                syntax_error("Invalid char literal escape '\\%c'.", *stream);
        }
        tb_stretchy_push(str, val);
        stream++;
    }

    if (*stream)
    {
        assert(*stream == '"');
        stream++;
    }
    else
    {
        syntax_error("Unexpected end of file within string literal");
    }

    tb_stretchy_push(str, '\0');
    token.type = TOKEN_STR;
    token.strval = str;
}

#define TOKEN_CASE1(c, c1, t1) \
    case c: \
        token.type = *stream++; \
        if (*stream == c1) { token.type = t1; stream++; } \
        break;

#define TOKEN_CASE2(c, c1, t1, c2, t2) \
    case c: \
        token.type = *stream++; \
        if (*stream == c1) { token.type = t1; stream++; } \
        else if (*stream == c2) { token.type = t2; stream++; } \
        break;

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
    case '\'':
        scan_char();
        break;
    case '"':
        scan_str();
        break;
    case '.':
        scan_float();
        break;
    case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
    {
        const char* check = stream;
        while (isdigit(*check))
            check++;

        if (*check == '.' || tolower(*check) == 'e')
            scan_float();
        else
            scan_int();
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
    case '<':
        token.type = *stream++;
        if (*stream == '<')
        {
            token.type = TOKEN_LSHIFT;
            stream++;
            if (*stream == '=')
            {
                token.type = TOKEN_LSHIFT_ASSIGN;
                stream++;
            }
        }
        else if (*stream == '=')
        {
            token.type = TOKEN_LTEQ;
            stream++;
        }
        break;
    case '>':
        token.type = *stream++;
        if (*stream == '>')
        {
            token.type = TOKEN_RSHIFT;
            stream++;
            if (*stream == '=')
            {
                token.type = TOKEN_RSHIFT_ASSIGN;
                stream++;
            }
        }
        else if (*stream == '=')
        {
            token.type = TOKEN_GTEQ;
            stream++;
        }
        break;

    TOKEN_CASE1(':', '=', TOKEN_COLON_ASSIGN)
    TOKEN_CASE1('^', '=', TOKEN_XOR_ASSIGN)
    TOKEN_CASE1('*', '=', TOKEN_MUL_ASSIGN)
    TOKEN_CASE1('/', '=', TOKEN_DIV_ASSIGN)
    TOKEN_CASE1('%', '=', TOKEN_MOD_ASSIGN)
    TOKEN_CASE2('+', '=', TOKEN_ADD_ASSIGN, '+', TOKEN_INC)
    TOKEN_CASE2('-', '=', TOKEN_SUB_ASSIGN, '-', TOKEN_DEC)
    TOKEN_CASE2('&', '=', TOKEN_AND_ASSIGN, '&', TOKEN_AND)
    TOKEN_CASE2('|', '=', TOKEN_OR_ASSIGN, '|', TOKEN_OR)
    default:
        token.type = *stream++;
        break;
    }
    token.end = stream;
}

#undef TOKEN_CASE1
#undef TOKEN_CASE2

void init_stream(const char* str)
{
    stream = str;
    next_token();
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
#define assert_token_int(x) assert(token.ival == (x) && match_token(TOKEN_INT))
#define assert_token_float(x) assert(token.fval == (x) && match_token(TOKEN_FLOAT))
#define assert_token_str(x) assert(strcmp(token.strval, (x)) == 0 && match_token(TOKEN_STR))
#define assert_token_eof() assert(match_token('\0'))

void lex_test()
{
    /* Integer literal tests */
    init_stream("0 0xffffffffffffffff 042 0b1111");
    assert_token_int(0);
    assert_token_int(0xffffffffffffffffull);
    assert_token_int(042);
    assert_token_int(0xF);
    assert_token_eof();

    /* Float literal tests */
    init_stream("3.14 .123 42. 3e10");
    assert_token_float(3.14);
    assert_token_float(.123);
    assert_token_float(42.);
    assert_token_float(3e10);
    assert_token_eof();

    /* Char literal tests */
    init_stream("'a' '\\n'");
    assert_token_int('a');
    assert_token_int('\n');
    assert_token_eof();

    /* String literal tests */
    init_stream("\"foo\"");
    assert_token_str("foo");
    assert_token_eof();

    /* operator tests */
    init_stream(": := + += ++ < <= << <<=");
    assert_token(':');
    assert_token(TOKEN_COLON_ASSIGN);
    assert_token('+');
    assert_token(TOKEN_ADD_ASSIGN);
    assert_token(TOKEN_INC);
    assert_token('<');
    assert_token(TOKEN_LTEQ);
    assert_token(TOKEN_LSHIFT);
    assert_token(TOKEN_LSHIFT_ASSIGN);
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

void expr_test()
{
    Expr* exprs[] = {
        expr_binary('+', expr_int(1), expr_int(2)),
        expr_unary('-', expr_float(3.14)),
        expr_ternary(expr_name("flag"), expr_str("true"), expr_str("false")),
        expr_field(expr_name("person"), "name"),
        expr_call(expr_name("fact"), (Expr*[]){ expr_int(42) }, 1),
        expr_index(expr_field(expr_name("person"), "siblings"), expr_int(3)),
        expr_cast(typespec_pointer(typespec_name("int")), expr_name("void_ptr"))
    };
    
    for (Expr** it = exprs; it != exprs + sizeof(exprs) / sizeof(*exprs); it++)
    {
        print_expr(*it);
        printf("\n");
    }
}

void run_tests()
{
    buf_test();

    str_intern_test();

    lex_test();

    parse_test();

    expr_test();
}

int main(int argc, char** argv)
{
    run_tests();

    return 0;
}