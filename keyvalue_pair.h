#ifndef CxKANOAXDP_keyvalue_pair_H_
#define CxKANOAXDP_keyvalue_pair_H_
#include "RAII.h"
#include <stdbool.h>
// key-value pair structure
typedef struct{
    RAII _;
    void* key;
    void* value;
    bool key_owned;
    bool value_owned;
}KeyValue;
// create a new key-value pair
//  ownership of key and value can be set separately
KeyValue* KeyValue_create(void* key, bool owns_key, void* value, bool owns_value);
#endif