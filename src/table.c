#include "table.h"

#include "memory.h"

#include <string.h>

#define CW_TABLE_MAX_LOAD 0.75

void cw_table_init(Table* table)
{
    table->size = 0;
    table->capacity = 0;
    table->entries = NULL;
}

void cw_table_free(Table* table)
{
    CW_FREE_ARRAY(TableEntry, table->entries, table->capacity);
    cw_table_init(table);
}

static uint32_t cw_find_entry(const TableEntry* entries, size_t cap, const cwString* key)
{
    int32_t tombstone = -1;
    uint32_t index = key->hash % cap;
    while (true)
    {
        /* entry is either empty or a tombstone */
        if (entries[index].key == NULL)
        {
            if (IS_NULL(entries[index].val)) return tombstone >= 0 ? tombstone : index; 
            
            if (tombstone < 0) tombstone = index;
        }
        else if (entries[index].key == key)
        {
            return index;
        }
        index = (index + 1) % cap;
    }
}

static void cw_table_grow(Table* table, int capacity)
{
    TableEntry* entries = CW_ALLOCATE(TableEntry, capacity);
    for (uint32_t i = 0; i < capacity; ++i)
    {
        entries[i].key = NULL;
        entries[i].val = MAKE_NULL();
    }

    /* moving entries of the old table to the new one */
    table->size = 0;
    for (uint32_t i = 0; i < table->capacity; ++i)
    {
        TableEntry* entry = &table->entries[i];
        if (entry->key == NULL) continue;

        uint32_t index = cw_find_entry(entries, capacity, entry->key);
        entries[index].key = entry->key;
        entries[index].val = entry->val;
        table->size++;
    }

    CW_FREE_ARRAY(TableEntry, table->entries, table->capacity);
    table->entries = entries;
    table->capacity = capacity;
}

bool cw_table_insert(Table* table, cwString* key, Value val)
{
    if (table->size + 1 > table->capacity * CW_TABLE_MAX_LOAD)
    {
        uint32_t capacity = CW_GROW_CAPACITY(table->capacity);
        cw_table_grow(table, capacity);
    }

    uint32_t i = cw_find_entry(table->entries, table->capacity, key);
    if (!table->entries[i].key && IS_NULL(table->entries[i].val)) table->size++;

    table->entries[i].key = key;
    table->entries[i].val = val;
    return table->entries[i].key == NULL;
}

bool cw_table_remove(Table* table, cwString* key)
{
    if (table->size == 0) return false;

    // Find the entry.
    uint32_t i = cw_find_entry(table->entries, table->capacity, key);
    if (!table->entries[i].key) return false;

    // Place a tombstone in the entry.
    table->entries[i].key = NULL;
    table->entries[i].val = MAKE_BOOL(true);
    return true;
}

bool cw_table_find(const Table* table, const cwString* key, Value* val)
{
    if (table->size == 0) return false;

    uint32_t i = cw_find_entry(table->entries, table->capacity, key);
    if (!table->entries[i].key) return false;

    *val = table->entries[i].val;
    return true;    
}

bool cw_table_copy(Table* src, Table* dst)
{
    for (int i = 0; i < src->capacity; i++)
    {
        TableEntry* entry = &src->entries[i];
        if (entry->key != NULL) cw_table_insert(dst, entry->key, entry->val);
    }
}

cwString* cw_table_find_key(const Table* table, const char* str, size_t len, uint32_t hash)
{
    if (table->size == 0) return NULL;

    uint32_t index = hash % table->capacity;
    while (true)
    {
        TableEntry* entry = &table->entries[index];

        /* Stop if we find an empty non-tombstone entry. */
        if (entry->key == NULL && IS_NULL(entry->val)) return NULL;
        
        /* Look for key with two early outs */
        if (entry->key->len == len && entry->key->hash == hash 
            && memcmp(entry->key->raw, str, len) == 0)
            return entry->key;

        index = (index + 1) % table->capacity;
    }
}