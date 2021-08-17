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

static TableEntry* cw_find_entry(const TableEntry* entries, size_t cap, const cwString* key)
{
    TableEntry* tombstone = NULL;
    uint32_t index = key->hash % cap;
    while (true)
    {
        TableEntry* entry = &entries[index];
        if (entry->key == NULL)
        {
            if (IS_NULL(entry->val)) return tombstone ? tombstone : entry; 
            
            if (!tombstone) tombstone = entry;
        }
        else if (entry->key == key)
        {
            return entry;
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

        TableEntry* dst = cw_find_entry(entries, capacity, entry->key);
        dst->key = entry->key;
        dst->val = entry->val;
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

    TableEntry* entry = cw_find_entry(table->entries, table->capacity, key);
    if (!entry->key && IS_NULL(entry->val)) table->size++;

    entry->key = key;
    return entry->key == NULL;
}

bool cw_table_remove(Table* table, cwString* key)
{
    if (table->size == 0) return false;

    // Find the entry.
    TableEntry* entry = cw_find_entry(table->entries, table->capacity, key);
    if (!entry->key) return false;

    // Place a tombstone in the entry.
    entry->key = NULL;
    entry->val = MAKE_BOOL(true);
    return true;
}

bool cw_table_find(const Table* table, const cwString* key, Value* val)
{
    if (table->size == 0) return false;

    TableEntry* entry = cw_find_entry(table->entries, table->capacity, key);
    if (!entry->key) return false;

    *val = entry->val;
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
        
        /* found key */
        if (entry->key->len == len && entry->key->hash == hash 
            && memcmp(entry->key->raw, str, len) == 0)
            return entry->key;

        index = (index + 1) % table->capacity;
    }
}