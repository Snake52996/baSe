#include "hashtable.h"
#include "RAII.h"
#include <stdlib.h>
static void HashTable_deleter_(void* target_);
static inline unsigned int HashTable_bounded_hash_(HashTable* hash_table, void* data){
    return hash_table->hash(data) % hash_table->lines;
}
void HashTable_initialize(
    HashTable* hash_table,
    size_t lines,
    unsigned int (*hash)(const void* target_),
    int (*compare)(const void* lhs_, const void* rhs_)
){
    hash_table->index = malloc(sizeof(List) * lines);
    hash_table->lines = lines;
    for(size_t i = 0; i < lines; i++){
        List_initialize(hash_table->index + i);
    }
    hash_table->size = 0;
    hash_table->hash = hash;
    hash_table->compare = compare;
    RAII_set_deleter(hash_table, HashTable_deleter_);
}
void HashTable_clear(HashTable* hash_table){
    for(size_t i = 0; i < hash_table->lines; i++){
        List_clear(hash_table->index + i);
    }
    hash_table->size = 0;
}
KeyValue* HashTable_find(HashTable* hash_table, void* key){
    unsigned int hash = HashTable_bounded_hash_(hash_table, key);
    List* target_link = &hash_table->index[hash];
    for(ListNode* target = target_link->head.next; target != &target_link->head; target = target->next){
        KeyValue* current = (KeyValue*)(target->data);
        if(hash_table->compare(key, current->key) == 0) return current;
    }
    return NULL;
}
void HashTable_erase_entry_key_hint(HashTable* hash_table, void* hint, KeyValue* entry){
    unsigned int hash = HashTable_bounded_hash_(hash_table, hint);
    List* target_link = &hash_table->index[hash];
    for(ListNode* target = target_link->head.next; target != &target_link->head; target = target->next){
        KeyValue* current = (KeyValue*)(target->data);
        if(current == entry){
            List_erase(target_link, target);
            break;
        }
    }
}
KeyValue* HashTable_insert_direct(
    HashTable* hash_table, void* key, bool owns_key, void* value, bool owns_value
){
    unsigned int hash = HashTable_bounded_hash_(hash_table, key);
    KeyValue* entry = KeyValue_create(key, owns_key, value, owns_value);
    ListNode* node = ListNode_create(entry, true);
    List_emplace_front(&hash_table->index[hash], node);
    return entry;
}
bool HashTable_insert(
    HashTable* hash_table, void* key, bool owns_key, void* value, bool owns_value,
    KeyValue** result
){
    KeyValue* find_result = HashTable_find(hash_table, key);
    if(find_result != NULL){
        if(result != NULL) *result = find_result;
        return false;
    }
    find_result = HashTable_insert_direct(hash_table, key, owns_key, value, owns_value);
    if(result != NULL) *result = find_result;
    return true;
}
static void HashTable_deleter_(void* target_){
    HashTable* target = target_;
    HashTable_clear(target);
    free(target->index);
    free(target);
}