#include "heap.h"
#include "keyvalue_pair.h"
#include "vector.h"
struct Heap{
    RAII _;
    Vector* data_;
    int (*compare_)(const void* lhs_, const void* rhs_);
};

// adjust the last element in heap upwards
static void Heap_swim_(Heap* heap){
    if(Vector_size(heap->data_) <= 1) return;
    unsigned int current_index = Vector_size(heap->data_) - 1;
    KeyValue* item = (KeyValue*)Vector_at(heap->data_, current_index);
    while(current_index != 0){
        unsigned int parent_index = ((current_index + 1) >> 1) - 1;
        KeyValue* parent_item = (KeyValue*)Vector_at(heap->data_, parent_index);
        if(heap->compare_(item->key, parent_item->key) < 0){
            Vector_swap(heap->data_, parent_index, current_index);
            current_index = parent_index;
        }else{
            break;
        }
    }
}

// adjust the first element in heap downwards
static void Heap_sink_(Heap* heap){
    if(Vector_size(heap->data_) <= 1) return;
    unsigned int current_index = 0;
    KeyValue* item = (KeyValue*)Vector_at(heap->data_, current_index);
    while(((current_index + 1) << 1) - 1 < Vector_size(heap->data_)){
        unsigned int right_child_index = (current_index + 1) << 1;
        unsigned int left_child_index = right_child_index - 1;
        unsigned int compare_target_index;
        KeyValue* compare_target_item = NULL;
        if(right_child_index >= Vector_size(heap->data_)){
            compare_target_index = left_child_index;
            compare_target_item = (KeyValue*)Vector_at(heap->data_, left_child_index);
        }else{
            KeyValue* left_child_item = (KeyValue*)Vector_at(heap->data_, left_child_index);
            KeyValue* right_child_item = (KeyValue*)Vector_at(heap->data_, right_child_index);
            if(heap->compare_(left_child_item->key, right_child_item->key) < 0){
                compare_target_index = left_child_index;
                compare_target_item = left_child_item;
            }else{
                compare_target_index = right_child_index;
                compare_target_item = right_child_item;
            }
        }
        if(heap->compare_(item->key, compare_target_item->key) > 0){
            Vector_swap(heap->data_, compare_target_index, current_index);
            current_index = compare_target_index;
        }else{
            break;
        }
    }
}

// destroy the heap
static void Heap_destroy_(Heap* heap){
    RAII_delete(heap->data_);
}

// initialize a heap
Heap* Heap_create(int (*compare)(const void* lhs_, const void* rhs_)){
    Heap* heap = (Heap*)malloc(sizeof(Heap));
    heap->data_ = Vector_create();
    heap->compare_ = compare;
    RAII_set_deleter(heap, (void(*)(void*))Heap_destroy_);
    return heap;
}

void Heap_clear(Heap* heap){
    Vector_clear(heap->data_);
}

unsigned int Heap_size(Heap* heap){
    return Vector_size(heap->data_);
}

void Heap_insert(Heap* heap, void* key, bool owns_key, void* value, bool owns_value){
    KeyValue* item = (KeyValue*)KeyValue_create(key, owns_key, value, owns_value);
    Vector_emplace_back(heap->data_, item, true);
    Heap_swim_(heap);
}

void Heap_pop(Heap* heap){
    if(Vector_empty(heap->data_)) return;
    Vector_swap(heap->data_, 0, Vector_size(heap->data_) - 1);
    Vector_pop_back(heap->data_);
    Heap_sink_(heap);
}

void* Heap_top(Heap* heap){
    if(Vector_empty(heap->data_)) return NULL;
    return ((KeyValue*)Vector_at(heap->data_, 0))->value;
}