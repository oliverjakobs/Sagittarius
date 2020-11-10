#ifndef LEX_H
#define LEX_H

#include "token.h"

#include "common.h"

void init_stream(const char* str);
void next_token();

bool is_token(TokenType type);
bool is_token_name(const char* name);
bool is_token_eof();

bool match_token(TokenType type);
bool expect_token(TokenType type);

Token* get_token();
TokenType get_token_type();

const char* token_info();

const char* typedef_keyword;
const char* enum_keyword;
const char* struct_keyword;
const char* union_keyword;
const char* var_keyword;
const char* const_keyword;
const char* func_keyword;
const char* sizeof_keyword;
const char* break_keyword;
const char* continue_keyword;
const char* return_keyword;
const char* if_keyword;
const char* else_keyword;
const char* while_keyword;
const char* do_keyword;
const char* for_keyword;
const char* switch_keyword;
const char* case_keyword;
const char* default_keyword;

const char* first_keyword;
const char* last_keyword;
const char** keywords;

void init_keywords();

bool is_keyword(const char* name);
bool is_keyword_name(const char* name);

bool match_keyword(const char* name);

bool is_unary_op();
bool is_mul_op();
bool is_add_op();
bool is_cmp_op();
bool is_assign_op();


#endif // !LEX_H
