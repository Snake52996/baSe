#include "keyvalue_pair.h"
// deleter for a key-value pair
static void KeyValue_deleter_(void* target_){
    KeyValue* target = (KeyValue*)target_;
    // deallocate the key if owned
    if(target->key_owned) RAII_delete(target->key);
    // deallocate the value if owned
    if(target->value_owned) RAII_delete(target->value);
}
KeyValue* KeyValue_create(void* key, bool owns_key, void* value, bool owns_value){
    KeyValue* entry = (KeyValue*)malloc(sizeof(KeyValue));
    entry->key = key;
    entry->value = value;
    entry->key_owned = owns_key;
    entry->value_owned = owns_value;
    RAII_set_deleter(entry, KeyValue_deleter_);
    return entry;
}