#include "table.h"

#include "memory.h"

#define CW_TABLE_MAX_LOAD 0.75

void cw_table_init(Table* table)
{
    table->size = 0;
    table->capacity = 0;
    table->keys = NULL;
    table->values = NULL;
}

void cw_table_free(Table* table)
{
    CW_FREE_ARRAY(cwString*, table->keys, table->capacity);
    CW_FREE_ARRAY(Value, table->values, table->capacity);
    cw_table_init(table);
}

static uint32_t cw_find_entry(cwString** keys, Value* values, size_t cap, cwString* find, bool* new_key)
{
    int32_t tombstone = -1;
    uint32_t index = find->hash % cap;
    while (true)
    {
        if (keys[index] == NULL)
        {
            if (new_key) *new_key = true;
            if (IS_NULL(values[index]))
                return tombstone >= 0 ? (uint32_t)tombstone : index; 
            
            if (tombstone < 0) tombstone = index;
        }
        else if (keys[index] == find)
        {
            if (new_key) *new_key = false;
            return index;
        }
        index = (index + 1) % cap;
    }
}

static void cw_table_rehash(Table* table, int capacity)
{
    cwString** keys = CW_ALLOCATE(cwString*, capacity);
    Value* values = CW_ALLOCATE(Value, capacity);
    for (uint32_t i = 0; i < capacity; ++i)
    {
        keys[i] = NULL;
        values[i] = MAKE_NULL();
    }

    /* moving entries of the old table to the new one */
    for (uint32_t i = 0; i < table->capacity; ++i)
    {
        if (table->keys[i] == NULL) continue;

        uint32_t index = cw_find_entry(keys, values, capacity, table->keys[i], NULL);
        keys[index] = table->keys[i];
        values[index] = table->values[i];
    }

    table->keys = keys;
    table->values = values;
    table->capacity = capacity;
}

bool cw_table_insert(Table* table, cwString* key, Value val)
{
    if (table->size + 1 > table->capacity * CW_TABLE_MAX_LOAD)
    {
        uint32_t capacity = CW_GROW_CAPACITY(table->capacity);
        cw_table_rehash(table, capacity);
    }

    bool new_key = false;
    uint32_t index = cw_find_entry(table->keys, table->values, table->capacity, key, &new_key);
    if (new_key) table->size++;

    table->keys[index] = key;
    table->values[index] = val;
    return new_key;
}

bool cw_table_find(Table* table, cwString* key, Value* val)
{
    if (table->size == 0) return false;

    bool new_key = false;
    uint32_t index = cw_find_entry(table->keys, table->values, table->capacity, key, &new_key);
    if (new_key) return false;

    *val = table->values[index];
    return true;    
}