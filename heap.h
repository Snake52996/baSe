#ifndef CxKANOAXDP_heap_H_
#define CxKANOAXDP_heap_H_
#include <stdbool.h>
// heap which support data of any type and any order specified when creating the heap
typedef struct Heap Heap;

// create a new heap
//  compare specifies the order by which the elements are arranged
Heap* Heap_create(int (*compare)(const void* lhs_, const void* rhs_));

// remove all elements from heap
void Heap_clear(Heap* heap);

// get number of elements in heap
inline unsigned int Heap_size(Heap* heap);

// insert a new element to heap
//  the owns_{key, value} parameter specify if the {key, value} shall be managed, that is generally about
//   deallocating the object. In case the object is managed, it must be a RAII object
void Heap_insert(Heap* heap, void* key, bool owns_key, void* value, bool owns_value);

// remove the element at the top of heap
void Heap_pop(Heap* heap);

// get the element at the top of heap
void* Heap_top(Heap* heap);
#endif