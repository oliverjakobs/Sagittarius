#include "ast/print.h"

#include "lex.h"
#include "parse.h"
#include "resolve.h"

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

void keyword_test()
{
    assert(is_keyword_name(first_keyword));
    assert(is_keyword_name(last_keyword));
    for (const char** it = keywords; it != tb_stretchy_last(keywords); it++)
    {
        assert(is_keyword_name(*it));
    }
    assert(!is_keyword_name(str_intern("foo")));
}

#define assert_token(x) assert(match_token(x))
#define assert_token_name(x) assert(get_token()->name == str_intern(x) && match_token(TOKEN_NAME))
#define assert_token_int(x) assert(get_token()->ival == (x) && match_token(TOKEN_INT))
#define assert_token_float(x) assert(get_token()->fval == (x) && match_token(TOKEN_FLOAT))
#define assert_token_str(x) assert(strcmp(get_token()->strval, (x)) == 0 && match_token(TOKEN_STR))
#define assert_token_eof() assert(match_token(TOKEN_EOF))

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
    assert_token(TOKEN_COLON);
    assert_token(TOKEN_COLON_ASSIGN);
    assert_token(TOKEN_PLUS);
    assert_token(TOKEN_ADD_ASSIGN);
    assert_token(TOKEN_INC);
    assert_token(TOKEN_LT);
    assert_token(TOKEN_LTEQ);
    assert_token(TOKEN_LSHIFT);
    assert_token(TOKEN_LSHIFT_ASSIGN);
    assert_token_eof();

    init_stream("XY+(XY)_HELLO1,234+994");
    assert_token_name("XY");
    assert_token(TOKEN_PLUS);
    assert_token(TOKEN_LPAREN);
    assert_token_name("XY");
    assert_token(TOKEN_RPAREN);
    assert_token_name("_HELLO1");
    assert_token(TOKEN_COMMA);
    assert_token_int(234);
    assert_token(TOKEN_PLUS);
    assert_token_int(994);
    assert_token_eof();
}

#undef assert_token
#undef assert_token_name
#undef assert_token_int
#undef assert_token_eof

void parse_test()
{
    const char* tests[] = {
        "const n = sizeof(:int*[16])",
        "const n = sizeof(1+2)",
        "var x = b == 1 ? 1+2 : 3-4",
        "func fact(n: int): int { trace(\"fact\"); if (n == 0) { return 1; } else { return n * fact(n-1); } }",
        "func fact(n: int): int { p := 1; for (i := 1; i <= n; i++) { p *= i; } return p; }",
        "var foo = a ? a&b + c<<d + e*f == +u-v-w + *g/h(x,y) + -i%k[x] && m <= n*(p+q)/r : 0",
        "func f(x: int): bool { switch(x) { case 0: case 1: return true; case 2: default: return false; } }",
        "enum Color { RED = 3, GREEN, BLUE = 0 }",
        "const pi = 3.14",
        "struct Vector { x, y: float; }",
        "var v = Vector{1.0, -1.0}",
        "var v: Vector = {1.0, -1.0}",
        "union IntOrFloat { i: int; f: float; }",
        "typedef Vectors = Vector[1+2]",
    };
    for (const char** it = tests; it != tests + sizeof(tests) / sizeof(*tests); it++)
    {
        init_stream(*it);
        Decl* decl = parse_decl();
        print_decl(decl);
        printf("\n");
    }
}

void expr_test()
{
    Expr* exprs[] = {
        expr_binary('+', expr_int(1), expr_int(2)),
        expr_unary('-', expr_float(3.14)),
        expr_ternary(expr_name("flag"), expr_str("true"), expr_str("false")),
        expr_field(expr_name("person"), "name"),
        expr_call(expr_name("fact"), (Expr*[]){ expr_int(42) }, 1),
        expr_index(expr_field(expr_name("person"), "siblings"), expr_int(3)),
        expr_cast(typespec_pointer(typespec_name("int")), expr_name("void_ptr")),
        expr_compound(typespec_name("Vector"), (Expr*[]) { expr_int(1), expr_int(2) }, 2)
    };
    
    for (Expr** it = exprs; it != exprs + sizeof(exprs) / sizeof(*exprs); it++)
    {
        print_expr(*it);
        printf("\n");
    }
}

void stmt_test()
{
    Stmt* stmts[] = {
        stmt_return(expr_int(42)),
        stmt_break(),
        stmt_continue(),
        stmt_block(
            (StmtList) {
                (Stmt*[]) {
                    stmt_break(),
                    stmt_continue()
                }, 2,
            }
        ),
        stmt_expr(expr_call(expr_name("print"), (Expr*[]) { expr_int(1), expr_int(2) }, 2)),
        stmt_auto("x", expr_int(42)),
        stmt_if(
            expr_name("flag1"),
            (StmtList) {
                (Stmt*[]) {
                    stmt_return(expr_int(1))
                }, 1,
            },
            (ElseIf[]) {
                expr_name("flag2"),
                (StmtList) {
                    (Stmt*[]) {
                        stmt_return(expr_int(2))
                    }, 1,
                }
            }, 1,
            (StmtList) {
                (Stmt*[]) {
                    stmt_return(expr_int(3))
                }, 1,
            }
        ),
        stmt_while(
            expr_name("running"),
            (StmtList) {
                (Stmt*[]) {
                    stmt_assign(TOKEN_ADD_ASSIGN, expr_name("i"), expr_int(16)),
                }, 1,
            }
        ),
        stmt_switch(
            expr_name("val"),
            (SwitchCase[]) { 
                {
                    (Expr*[]) { expr_int(3), expr_int(4) }, 2, false,
                    (StmtList) {
                        (Stmt*[]) {
                            stmt_return(expr_name("val"))
                        }, 1,
                    },
                },
                {
                    (Expr*[]){ expr_int(1) }, 1, true,
                    (StmtList) {
                        (Stmt*[]) {
                            stmt_return(expr_int(0))
                        }, 1,
                    },
                },
            }, 2
        ),
    };

    for (Stmt** it = stmts; it != stmts + sizeof(stmts) / sizeof(*stmts); it++)
    {
        print_stmt(*it);
        printf("\n");
    }
}

void resolve_test()
{
    Type* int_ptr = type_pointer(type_int());
    assert(type_pointer(type_int()) == int_ptr);
    Type* float_ptr = type_pointer(type_float());
    assert(type_pointer(type_float()) == float_ptr);
    assert(int_ptr != float_ptr);
    Type* int_ptr_ptr = type_pointer(type_pointer(type_int()));
    assert(type_pointer(type_pointer(type_int())) == int_ptr_ptr);
    Type* float4_array = type_array(type_float(), 4);
    assert(type_array(type_float(), 4) == float4_array);
    Type* float3_array = type_array(type_float(), 3);
    assert(type_array(type_float(), 3) == float3_array);
    assert(float4_array != float3_array);
    Type* int_int_func = type_func((Type*[]) { type_int() }, 1, type_int());
    assert(type_func((Type*[]) { type_int() }, 1, type_int()) == int_int_func);
    Type* int_func = type_func(NULL, 0, type_int());
    assert(int_int_func != int_func);
    assert(int_func == type_func(NULL, 0, type_int()));

    const char* int_name = str_intern("int");
    entity_install_type(int_name, type_int());
    const char* code[] = {
        "const n = 1+sizeof(p)",
        "var p: T*",
        "var u = *p",
        "struct T { a: int[n]; }",
        "var r = &t.a",
        "var t: T",
        "typedef S = int[n+m]",
        "const m = sizeof(t.a)",
        "var i = n+m",
        "var q = &i",
        //        "const n = sizeof(x)",
        //        "var x: T",
        //        "struct T { s: S*; }",
        //        "struct S { t: T[n]; }",
    };
    for (size_t i = 0; i < sizeof(code) / sizeof(*code); i++)
    {
        init_stream(code[i]);
        Decl* decl = parse_decl();
        entity_install_decl(decl);
    }
    for (Entity** it = get_entities(); it != tb_stretchy_last(get_entities()); it++)
    {
        Entity* entity = *it;
        complete_entity(entity);
    }
    for (Entity** it = get_ordered_entities(); it != tb_stretchy_last(get_ordered_entities()); it++)
    {
        Entity* entity = *it;
        if (entity->decl)
            print_decl(entity->decl);
        else
            printf("%s", entity->name);

        printf("\n");
    }
}

void run_tests()
{
    buf_test();
    str_intern_test();

    init_keywords();
    keyword_test();

    lex_test();

    /*
    parse_test();

    expr_test();
    stmt_test();
    */

    resolve_test();
}

int main(int argc, char** argv)
{
    run_tests();

    return 0;
}