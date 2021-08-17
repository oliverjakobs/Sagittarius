#ifndef CW_TABLE_H
#define CW_TABLE_H

#include "common.h"

typedef struct
{
    cwString* key;
    Value     val;
} TableEntry;

typedef struct
{
    uint32_t size;
    uint32_t capacity;
    TableEntry* entries;
} Table;

void cw_table_init(Table* table);
void cw_table_free(Table* table);

bool cw_table_insert(Table* table, cwString* key, Value val);
bool cw_table_remove(Table* table, cwString* key);
bool cw_table_find(const Table* table, const cwString* key, Value* val);

bool cw_table_copy(Table* src, Table* dst);
cwString* cw_table_find_key(const Table* table, const char* str, size_t len, uint32_t hash);

#endif /* !CW_TABLE_H */