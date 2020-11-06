#ifndef AST_H
#define AST_H

#include "common.h"

#include "token.h"

typedef struct Expr Expr;
typedef struct Decl Decl;
typedef struct Stmt Stmt;

typedef struct
{
    Stmt** stmts;
    size_t num_stmts;
} StmtBlock;

typedef struct Typespec Typespec;

typedef enum
{
    TYPESPEC_NONE,
    TYPESPEC_NAME,
    TYPESPEC_FUNC,
    TYPESPEC_ARRAY,
    TYPESPEC_POINTER,
} TypespecType;

typedef struct
{
    Typespec** args;
    size_t num_args;
    Typespec* ret;
} FuncTypespec;

typedef struct
{
    Typespec* elem;
} PointerTypespec;

typedef struct
{
    Typespec* elem;
    Expr* size;
} ArrayTypespec;

struct Typespec
{
    TypespecType type;
    union
    {
        const char* name;
        FuncTypespec func;
        ArrayTypespec array;
        PointerTypespec ptr;
    };
};

Typespec* typespec_name(const char* name);
Typespec* typespec_pointer(Typespec* elem);
Typespec* typespec_array(Typespec* elem, Expr* size);
Typespec* typespec_func(Typespec** args, size_t num_args, Typespec* ret);

typedef enum
{
    DECL_NONE,
    DECL_ENUM,
    DECL_STRUCT,
    DECL_UNION,
    DECL_VAR,
    DECL_CONST,
    DECL_TYPEDEF,
    DECL_FUNC,
} DeclType;

typedef struct
{
    const char* name;
    Typespec* type;
} FuncParam;

typedef struct
{
    FuncParam* params;
    size_t num_params;
    Typespec* ret_type;
    StmtBlock block;
} FuncDecl;

typedef struct
{
    const char* name;
    Expr* init;
} EnumItem;

typedef struct
{
    EnumItem* items;
    size_t num_items;
} EnumDecl;

typedef struct
{
    const char** names;
    size_t num_names;
    Typespec* type;
} AggregateItem;

typedef struct
{
    AggregateItem* items;
    size_t num_items;
} AggregateDecl;

typedef struct
{
    Typespec* type;
} TypedefDecl;

typedef struct
{
    Typespec* type;
    Expr* expr;
} VarDecl;

typedef struct
{
    Expr* expr;
} ConstDecl;

struct Decl
{
    DeclType type;
    const char* name;
    union
    {
        EnumDecl enum_decl;
        AggregateDecl aggregate_decl;
        FuncDecl func_decl;
        TypedefDecl typedef_decl;
        VarDecl var_decl;
        ConstDecl const_decl;
    };
};

Decl* decl_enum(const char* name, EnumItem* items, size_t num_items);
Decl* decl_struct(const char* name, AggregateItem* items, size_t num_items);
Decl* decl_union(const char* name, AggregateItem* items, size_t num_items);
Decl* decl_var(const char* name, Typespec* type, Expr* expr);
Decl* decl_func(const char* name, FuncParam* params, size_t num_params, Typespec* ret_type, StmtBlock block);
Decl* decl_const(const char* name, Expr* expr);
Decl* decl_typedef(const char* name, Typespec* type);

typedef enum
{
    EXPR_NONE,
    EXPR_INT,
    EXPR_FLOAT,
    EXPR_STR,
    EXPR_NAME,
    EXPR_CAST,
    EXPR_CALL,
    EXPR_INDEX,
    EXPR_FIELD,
    EXPR_COMPOUND,
    EXPR_UNARY,
    EXPR_BINARY,
    EXPR_TERNARY,
} ExprType;

typedef struct
{
    Typespec* type;
    Expr** args;
    size_t num_args;
} CompoundExpr;

typedef struct
{
    Typespec* type;
    Expr* expr;
} CastExpr;

typedef struct
{
    TokenType op;
    Expr* expr;
} UnaryExpr;

typedef struct
{
    TokenType op;
    Expr* left;
    Expr* right;
} BinaryExpr;

typedef struct
{
    Expr* cond;
    Expr* then_expr;
    Expr* else_expr;
} TernaryExpr;

typedef struct
{
    Expr* expr;
    Expr** args;
    size_t num_args;
} CallExpr;

typedef struct
{
    Expr* expr;
    Expr* index;
} IndexExpr;

typedef struct
{
    Expr* expr;
    const char* name;
} FieldExpr;

struct Expr
{
    ExprType type;
    union
    {
        uint64_t ival;
        double fval;
        const char* strval;
        const char* name;
        CompoundExpr compound;
        CastExpr cast;
        UnaryExpr unary;
        BinaryExpr binary;
        TernaryExpr ternary;
        CallExpr call;
        IndexExpr index;
        FieldExpr field;
    };
};

Expr* expr_int(uint64_t val);
Expr* expr_float(double val);
Expr* expr_str(const char* str);
Expr* expr_name(const char* name);
Expr* expr_cast(Typespec* type, Expr* expr);
Expr* expr_call(Expr* operand, Expr** args, size_t num_args);
Expr* expr_index(Expr* operand, Expr* index);
Expr* expr_field(Expr* operand, const char* field);
Expr* expr_compound(Typespec* type, Expr** args, size_t num_args);
Expr* expr_unary(TokenType op, Expr* expr);
Expr* expr_binary(TokenType op, Expr* left, Expr* right);
Expr* expr_ternary(Expr* cond, Expr* then_expr, Expr* else_expr);

typedef enum
{
    STMT_NONE,
    STMT_RETURN,
    STMT_BREAK,
    STMT_CONTINUE,
    STMT_BLOCK,
    STMT_IF,
    STMT_WHILE,
    STMT_DO_WHILE,
    STMT_FOR,
    STMT_SWITCH,
    STMT_ASSIGN,
    STMT_INIT,
    STMT_EXPR,
} StmtType;

typedef struct
{
    Expr* expr;
} ReturnStmt;

typedef struct
{
    Expr* cond;
    StmtBlock block;
} ElseIf;

typedef struct
{
    Expr* cond;
    StmtBlock then_block;
    ElseIf* elseifs;
    size_t num_elseifs;
    StmtBlock else_block;
} IfStmt;

typedef struct
{
    Expr* cond;
    StmtBlock block;
} WhileStmt;

typedef struct
{
    StmtBlock init;
    Expr* cond;
    StmtBlock next;
    StmtBlock block;
} ForStmt;

typedef struct
{
    Expr** exprs;
    size_t num_exprs;
    bool is_default;
    StmtBlock block;
} SwitchCase;

typedef struct
{
    Expr* expr;
    SwitchCase* cases;
    size_t num_cases;
} SwitchStmt;

typedef struct
{
    TokenType op;
    Expr* left;
    Expr* right;
} AssignStmt;

typedef struct
{
    const char* name;
    Expr* expr;
} InitStmt;

struct Stmt
{
    StmtType type;
    union
    {
        ReturnStmt return_stmt;
        IfStmt if_stmt;
        WhileStmt while_stmt;
        ForStmt for_stmt;
        SwitchStmt switch_stmt;
        StmtBlock block;
        AssignStmt assign;
        InitStmt init;
        Expr* expr;
    };
};

Stmt* stmt_return(Expr* expr);
Stmt* stmt_break();
Stmt* stmt_continue();
Stmt* stmt_block(StmtBlock block);
Stmt* stmt_if(Expr* cond, StmtBlock then_block, ElseIf* elseifs, size_t num_elseifs, StmtBlock else_block);
Stmt* stmt_while(Expr* cond, StmtBlock block);
Stmt* stmt_do_while(Expr* cond, StmtBlock block);
Stmt* stmt_for(StmtBlock init, Expr* cond, StmtBlock next, StmtBlock block);
Stmt* stmt_switch(Expr* expr, SwitchCase* cases, size_t num_cases);
Stmt* stmt_assign(TokenType op, Expr* left, Expr* right);
Stmt* stmt_init(const char* name, Expr* expr);
Stmt* stmt_expr(Expr* expr);

#endif /* !AST_H */
