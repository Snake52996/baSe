#include "RAII.h"
#include "vector.h"
enum Constant{
    VectorInitialCapability = 32,   // initial capability of vector
    VectorEnlargeBias = 10,         // bias of enlarging the vector capability
};
typedef struct{
    RAII _;
    void* data;
}VectorItem_;
static void VectorItem_deleter_(void* target_){
    RAII_delete(((VectorItem_*)target_)->data);
}
static VectorItem_* VectorItem_create_(void* data, bool owned){
    VectorItem_* item = (VectorItem_*)malloc(sizeof(VectorItem_));
    item->data = data;
    if(owned) RAII_set_deleter(item, VectorItem_deleter_);
    else RAII_set_dummy_deleter(item);
    return item;
}
struct Vector{
    RAII _;
    VectorItem_** data;
    size_t size;
    size_t capability;
};
// destroy a vector
static void Vector_destroy_(Vector* vector){
    Vector_clear(vector);
    free(vector->data);
    vector->data = NULL;
    vector->size = 0;
    vector->capability = 0;
}
// initialize vector
static inline void Vector_initialize_(Vector* vector){
    vector->data = (VectorItem_**)malloc(sizeof(VectorItem_*) * VectorInitialCapability);
    vector->size = 0;
    vector->capability = VectorInitialCapability;
    RAII_set_deleter(vector, (void(*)(void*))Vector_destroy_);
}
Vector* Vector_create(){
    Vector* vector = (Vector*)malloc(sizeof(Vector));
    Vector_initialize_(vector);
    return vector;
}
void Vector_clear(Vector* vector){
    for(unsigned int i = 0; i < vector->size; i++){
        RAII_delete(vector->data[i]);
    }
    if(vector->capability >= VectorInitialCapability){
        vector->data = (VectorItem_**)realloc(vector->data, sizeof(VectorItem_*) * VectorInitialCapability);
    }else{
        // avoid unwanted copy of memory content
        free(vector->data);
        vector->data = (VectorItem_**)malloc(sizeof(VectorItem_*) * VectorInitialCapability);
    }
    vector->size = 0;
    vector->capability = VectorInitialCapability;
}

size_t Vector_size(Vector* vector){
    return vector->size;
}

bool Vector_empty(Vector* vector){
    return Vector_size(vector) == 0;
}

void Vector_recap(Vector* vector, size_t new_capability){
    if(new_capability == vector->capability || new_capability < vector->size) return;
    vector->data = (VectorItem_**)realloc(vector->data, sizeof(VectorItem_*) * new_capability);
    vector->capability = new_capability;
}

void Vector_emplace_back(Vector* vector, void* data, bool owned){
    if(vector->size == vector->capability){
        Vector_recap(vector, (vector->capability << 1) + VectorEnlargeBias);
    }
    VectorItem_* item = VectorItem_create_(data, owned);
    vector->data[vector->size] = item;
    vector->size += 1;
}

void Vector_pop_back(Vector* vector){
    if(vector->size == 0) return;
    vector->size -= 1;
    RAII_delete(vector->data[vector->size]);
    if(vector->size < (vector->capability >> 1)) Vector_recap(vector, vector->capability >> 1);
}

void Vector_swap(Vector* vector, size_t p, size_t q){
    if(p >= vector->size || q >= vector->size) return;
    VectorItem_* temp = vector->data[p];
    vector->data[p] = vector->data[q];
    vector->data[q] = temp;
}

void* Vector_at(Vector* vector, size_t index){
    if(index >= vector->size) return NULL;
    return vector->data[index]->data;
}

void Vector_foreach(Vector* vector, void(*function)(void* element, va_list ap), ...){
    const size_t size = Vector_size(vector);
    va_list ap;
    for(size_t i = 0; i < size; i++){
        va_start(ap, function);
        function(Vector_at(vector, i), ap);
        va_end(ap);
    }
}