

#include "../src/lexer.c"

#include <stdio.h>
#include <stdlib.h>

void assert_token(Token expected, Token actual) {
    if (expected.type == actual.type
      && strncmp(expected.start, actual.start, actual.end - actual.start) == 0) {
        return;
    }

    printf("Expected ");
    print_token(expected);
    printf(", but got ");
    print_token(actual);
    printf("\n");
    exit(1);
}

void test_next_token() {
    const char* input = 
    "let five = 5;"
    "let ten = 10;"
    ""
    "let add = func(x, y) {"
    "    x + y;"
    "};"
    ""
    "let result = add(five, ten);";

    Token expected[] = {
        { TOKEN_LET, "let"},
        { TOKEN_IDENTIFIER, "five"},
        { TOKEN_ASSIGN, "="},
        { TOKEN_NUMBER, "5"},
        { TOKEN_SEMICOLON, ";"},
        { TOKEN_LET, "let"},
        { TOKEN_IDENTIFIER, "ten"},
        { TOKEN_ASSIGN, "="},
        { TOKEN_NUMBER, "10"},
        { TOKEN_SEMICOLON, ";"},
        { TOKEN_LET, "let"},
        { TOKEN_IDENTIFIER, "add"},
        { TOKEN_ASSIGN, "="},
        { TOKEN_FUNC, "func"},
        { TOKEN_LPAREN, "("},
        { TOKEN_IDENTIFIER, "x"},
        { TOKEN_COMMA, ","},
        { TOKEN_IDENTIFIER, "y"},
        { TOKEN_RPAREN, ")"},
        { TOKEN_LBRACE, "{"},
        { TOKEN_IDENTIFIER, "x"},
        { TOKEN_PLUS, "+"},
        { TOKEN_IDENTIFIER, "y"},
        { TOKEN_SEMICOLON, ";"},
        { TOKEN_RBRACE, "}"},
        { TOKEN_SEMICOLON, ";"},
        { TOKEN_LET, "let"},
        { TOKEN_IDENTIFIER, "result"},
        { TOKEN_ASSIGN, "="},
        { TOKEN_IDENTIFIER, "add"},
        { TOKEN_LPAREN, "("},
        { TOKEN_IDENTIFIER, "five"},
        { TOKEN_COMMA, ","},
        { TOKEN_IDENTIFIER, "ten"},
        { TOKEN_RPAREN, ")"},
        { TOKEN_SEMICOLON, ";"},
        { TOKEN_EOF, ""},
        { TOKEN_ASSIGN, "=" },
        { TOKEN_PLUS, "+" },
        { TOKEN_LPAREN, "(" },
        { TOKEN_RPAREN, ")" },
        { TOKEN_LBRACE, "{" },
        { TOKEN_RBRACE, "}" },
        { TOKEN_COMMA, "," },
        { TOKEN_SEMICOLON, ";" },
        { TOKEN_EOF, "" }
    };

    Token token;
    const char* cursor = input;
    int line = 0;
    int index = 0;
    do {
        cursor = next_token(cursor, &line, &token);
        assert_token(expected[index++], token);

    } while (token.type != TOKEN_EOF && token.type != TOKEN_UNKOWN);
}



int main(int argc, const char* argv[]) {
    printf("Running Lexer Tests:\n");

    test_next_token();

    printf("Success.\n");

    return 0;
}