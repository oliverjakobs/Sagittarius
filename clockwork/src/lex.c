#include "lex.h"

static Token token;
static const char* stream;

void init_stream(const char* str)
{
    stream = str;
    next_token();
}

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
repeat:
    token.start = stream;
    token.mod = 0;
    switch (*stream) {
    case ' ': case '\n': case '\r': case '\t': case '\v':
        while (isspace(*stream))
            stream++;

        goto repeat;
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
    case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9': {
        while (isdigit(*stream)) {
            stream++;
        }
        char c = *stream;
        stream = token.start;
        if (c == '.' || tolower(c) == 'e')
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
        while (isalnum(*stream) || *stream == '_')
            stream++;

        token.name = str_intern_range(token.start, stream);
        token.type = is_keyword_str(token.name) ? TOKEN_KEYWORD : TOKEN_NAME;
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
    TOKEN_CASE1('=', '=', TOKEN_EQ)
    TOKEN_CASE1('^', '=', TOKEN_XOR_ASSIGN)
    TOKEN_CASE1(':', '=', TOKEN_COLON_ASSIGN)
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

bool is_token(TokenType type)
{
    return token.type == type;
}

bool is_token_name(const char* name)
{
    return token.type == TOKEN_NAME && token.name == name;
}

bool is_token_eof()
{
    return token.type == TOKEN_EOF;
}

bool match_token(TokenType type)
{
    if (!is_token(type)) return false;

    next_token();
    return true;
}

bool expect_token(TokenType type)
{
    if (!is_token(type))
    {
        char buf[256];
        copy_token_type_str(buf, sizeof(buf), type);
        fatal("expected token %s, got %s", buf, temp_token_type_str(token.type));
        return false;
    }

    next_token();
    return true;
}

Token* get_token()
{
    return &token;
}

TokenType get_token_type()
{
    return token.type;
}


#define KEYWORD(name) name##_keyword = str_intern(#name); tb_stretchy_push(keywords, name##_keyword)
void init_keywords()
{
    static bool inited;
    if (inited)
        return;

    char* arena_end = get_str_arena()->end;
    KEYWORD(typedef);
    KEYWORD(enum);
    KEYWORD(struct);
    KEYWORD(union);
    KEYWORD(const);
    KEYWORD(var);
    KEYWORD(func);
    KEYWORD(sizeof);
    KEYWORD(break);
    KEYWORD(continue);
    KEYWORD(return);
    KEYWORD(if);
    KEYWORD(else);
    KEYWORD(while);
    KEYWORD(do);
    KEYWORD(for);
    KEYWORD(switch);
    KEYWORD(case);
    KEYWORD(default);
    assert(get_str_arena()->end == arena_end);
    first_keyword = typedef_keyword;
    last_keyword = default_keyword;
    inited = true;
}

#undef KEYWORD

bool is_keyword(const char* name)
{
    return is_token(TOKEN_KEYWORD) && token.name == name;
}

bool is_keyword_str(const char* str)
{
    return first_keyword <= str && str <= last_keyword;
}

bool match_keyword(const char* name)
{
    if (is_keyword(name))
    {
        next_token();
        return true;
    }
    else
    {
        return false;
    }
}

bool is_unary_op()
{
    return is_token('+') || is_token('-') || is_token('*') || is_token('&');
}

bool is_mul_op()
{
    return is_token('*') || is_token('/') || is_token('%') || is_token('&') 
        || is_token(TOKEN_LSHIFT) || is_token(TOKEN_RSHIFT);
}

bool is_add_op()
{
    return is_token('+') || is_token('-') || is_token('|') || is_token('^');
}

bool is_cmp_op()
{
    return is_token('<') || is_token('>') || is_token(TOKEN_EQ) || is_token(TOKEN_NOTEQ) 
        || is_token(TOKEN_GTEQ) || is_token(TOKEN_LTEQ);
}

bool is_assign_op()
{
    return TOKEN_FIRST_ASSIGN <= token.type && token.type <= TOKEN_LAST_ASSIGN;
}