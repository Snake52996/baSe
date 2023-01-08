#ifndef CxKANOAXDP_Hashtable_H_
#define CxKANOAXDP_Hashtable_H_
#include "RAII.h"
#include "keyvalue_pair.h"
#include "list.h"
#include <stdbool.h>
typedef struct{
    RAII _;
    List* index;
    size_t size;
    size_t lines;
    unsigned int (*hash)(const void* key);
    int (*compare)(const void* lhs_, const void* rhs_);
}HashTable;

// initialize hashtable
void HashTable_initialize(
    HashTable* hash_table,
    size_t lines,
    unsigned int (*hash)(const void* target_),
    int (*compare)(const void* lhs_, const void* rhs_)
);

// clear up hash table
void HashTable_clear(HashTable* hash_table);

// lookup entry within hashtable
//  if multiple entries with same key exists, the last one inserted will be returned
KeyValue* HashTable_find(HashTable* hash_table, void* key);

// insert a new entry to hashtable, ignore any key conflict that may happen
KeyValue* HashTable_insert_direct(
    HashTable* hash_table, void* key, bool owns_key, void* value, bool owns_value
);

// remove a entry from hashtable with key hint
void HashTable_erase_entry_key_hint(HashTable* hash_table, void* hint, KeyValue* entry);

// insert a new entry to hashtable
bool HashTable_insert(
    HashTable* hash_table, void* key, bool owns_key, void* value, bool owns_value,
    KeyValue** result
);
#endif