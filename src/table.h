#ifndef CW_TABLE_H
#define CW_TABLE_H

#include "common.h"

typedef struct
{
  uint32_t size;
  uint32_t capacity;
  cwString** keys;
  Value* values;
} Table;

void cw_table_init(Table* table);
void cw_table_free(Table* table);

bool cw_table_insert(Table* table, cwString* key, Value val);
bool cw_table_find(Table* table, cwString* key, Value* val);

#endif /* !CW_TABLE_H */