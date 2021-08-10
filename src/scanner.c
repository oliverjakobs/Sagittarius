#include "scanner.h"

#include <string.h>
#include <stdio.h>

#include "common.h"

void cw_scanner_init(Scanner* scanner, const char* src)
{
    scanner->start = src;
    scanner->current = src;
    scanner->line = 1;
}

static inline bool cw_isdigit(char c) { return c >= '0' && c <= '9'; }
static inline bool cw_isalpha(char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_'; }

static inline bool cw_isend(const Scanner* scanner)     { return *scanner->current == '\0'; }
static inline char cw_peek(const Scanner* scanner)      { return *scanner->current; }
static inline char cw_peek_next(const Scanner* scanner) { return scanner->current[1]; }
static inline char cw_advance(Scanner* scanner)         { scanner->current++; return *(scanner->current-1); }

static inline int cw_scanner_offset(const Scanner* scanner) { return scanner->current - scanner->start; }

static bool cw_match(Scanner* scanner, char expected)
{
    if (cw_isend(scanner) || *scanner->current != expected) return false;
    scanner->current++;
    return true;
}

static void cw_skip_whitespaces(Scanner* scanner)
{
    while (true)
    {
        switch (cw_peek(scanner))
        {
        case '\n':
            scanner->line++;
        case ' ': case '\t': case '\r':
            cw_advance(scanner);
            break;
        case '#':
            while (!cw_isend(scanner) && cw_peek(scanner) != '\n') cw_advance(scanner);
            break;
        default:
            return;
        }
    }
}

static Token cw_make_token(const Scanner* scanner, TokenType type)
{
    Token token;
    token.type = type;
    token.start = scanner->start;
    token.length = cw_scanner_offset(scanner);
    token.line = scanner->line;
    return token;
}

static Token cw_make_error(const Scanner* scanner, const char* message)
{
    Token token;
    token.type = TOKEN_ERROR;
    token.start = message;
    token.length = strlen(message);
    token.line = scanner->line;
    return token;
}

static Token cw_make_string(Scanner* scanner)
{
    while (cw_peek(scanner) != '"')
    {
        if (cw_isend(scanner)) return cw_make_error(scanner, "Unterminated string.");
        if (cw_peek(scanner) == '\n') scanner->line++;
        cw_advance(scanner);
    }

    cw_advance(scanner); /* skip the closing quote */
    return cw_make_token(scanner, TOKEN_STRING);
}

static Token cw_make_number(Scanner* scanner)
{
    while (cw_isdigit(cw_peek(scanner))) cw_advance(scanner);

    // Look for a fractional part.
    if (cw_peek(scanner) == '.' && cw_isdigit(cw_peek_next(scanner)))
    {
        cw_advance(scanner); /* Consume the ".". */
        while (cw_isdigit(cw_peek(scanner))) cw_advance(scanner);
    }
  
    return cw_make_token(scanner, TOKEN_NUMBER);
}

static TokenType cw_check_keyword(Scanner* scanner, int offset, int len, const char* rest, TokenType type)
{
    if (cw_scanner_offset(scanner) != offset + len && memcmp(scanner->start + offset, rest, len) == 0)
        return type;
    return TOKEN_IDENTIFIER;
}

static TokenType cw_identifier_type(Scanner* scanner)
{
    switch (scanner->start[0])
    {
    case 'a': return cw_check_keyword(scanner, 1, 2, "nd", TOKEN_AND);
    case 'c': return cw_check_keyword(scanner, 1, 4, "lass", TOKEN_CLASS);
    case 'e': return cw_check_keyword(scanner, 1, 3, "lse", TOKEN_ELSE);
    case 'i': return cw_check_keyword(scanner, 1, 1, "f", TOKEN_IF);
    case 'f':
        if (cw_scanner_offset(scanner) > 1)
        {
            switch (scanner->start[1])
            {
                case 'a': return cw_check_keyword(scanner, 2, 3, "lse", TOKEN_FALSE);
                case 'o': return cw_check_keyword(scanner, 2, 1, "r", TOKEN_FOR);
                case 'u': return cw_check_keyword(scanner, 2, 6, "nction", TOKEN_FUNC);
            }
        }
        break;
    case 'l': return cw_check_keyword(scanner, 1, 2, "et", TOKEN_DECLARE);
    case 'n': return cw_check_keyword(scanner, 1, 3, "ull", TOKEN_NULL);
    case 'o': return cw_check_keyword(scanner, 1, 1, "r", TOKEN_OR);
    case 'p': return cw_check_keyword(scanner, 1, 4, "rint", TOKEN_PRINT);
    case 'r': return cw_check_keyword(scanner, 1, 5, "eturn", TOKEN_RETURN);
    case 's': return cw_check_keyword(scanner, 1, 4, "uper", TOKEN_SUPER);
    case 't':
        if (cw_scanner_offset(scanner) > 1)
        {
            switch (scanner->start[1])
            {
                case 'h': return cw_check_keyword(scanner, 2, 2, "is", TOKEN_THIS);
                case 'r': return cw_check_keyword(scanner, 2, 2, "ue", TOKEN_TRUE);
            }
        }
      break;
    case 'w': return cw_check_keyword(scanner, 1, 4, "hile", TOKEN_WHILE);
    }

    return TOKEN_IDENTIFIER;
}

static Token cw_make_identifier(Scanner* scanner)
{
    while (cw_isalpha(cw_peek(scanner)) || cw_isdigit(cw_peek(scanner))) cw_advance(scanner);
    return cw_make_token(scanner, cw_identifier_type(scanner));
}

Token cw_scan_token(Scanner* scanner)
{
    cw_skip_whitespaces(scanner);
    scanner->start = scanner->current;

    if (cw_isend(scanner)) return cw_make_token(scanner, TOKEN_EOF);

    switch (cw_advance(scanner))
    {
    case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
        return cw_make_number(scanner);
    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g': case 'h': case 'i': case 'j':
    case 'k': case 'l': case 'm': case 'n': case 'o': case 'p': case 'q': case 'r': case 's': case 't':
    case 'u': case 'v': case 'w': case 'x': case 'y': case 'z':
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G': case 'H': case 'I': case 'J':
    case 'K': case 'L': case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
    case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
    case '_':
        return cw_make_identifier(scanner);
    case '(': return cw_make_token(scanner, TOKEN_LPAREN);
    case ')': return cw_make_token(scanner, TOKEN_RPAREN);
    case '{': return cw_make_token(scanner, TOKEN_LBRACE);
    case '}': return cw_make_token(scanner, TOKEN_RBRACE);
    case ';': return cw_make_token(scanner, TOKEN_SEMICOLON);
    case ',': return cw_make_token(scanner, TOKEN_COMMA);
    case '.': return cw_make_token(scanner, TOKEN_PERIOD);
    case '-': return cw_make_token(scanner, TOKEN_MINUS);
    case '+': return cw_make_token(scanner, TOKEN_PLUS);
    case '/': return cw_make_token(scanner, TOKEN_SLASH);
    case '*': return cw_make_token(scanner, TOKEN_ASTERISK);
    case '!': return cw_make_token(scanner, cw_match(scanner, '=') ? TOKEN_NOTEQ : TOKEN_EXCLAMATION);
    case '=': return cw_make_token(scanner, cw_match(scanner, '=') ? TOKEN_EQ : TOKEN_ASSIGN);
    case '<': return cw_make_token(scanner, cw_match(scanner, '=') ? TOKEN_LTEQ : TOKEN_LT); 
    case '>': return cw_make_token(scanner, cw_match(scanner, '=') ? TOKEN_GTEQ : TOKEN_GT);
    }

    return cw_make_error(scanner, "Unexpected character.");
}