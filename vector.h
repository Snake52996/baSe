#ifndef CxKANOAXDP_vector_H_
#define CxKANOAXDP_vector_H_
#include <stdbool.h>
#include <stddef.h>
#include <stdarg.h>
// vector, an array of dynamic size with the ability to store elements of any type
typedef struct Vector Vector;

// create a new vector
Vector* Vector_create();

// clear elements in vector
void Vector_clear(Vector* vector);

// get number of elements in vector
inline size_t Vector_size(Vector* vector);

// check if there is no element in vector
inline bool Vector_empty(Vector* vector);

// adjust capability of vector
void Vector_recap(Vector* vector, size_t new_capability);

// emplace a element at the end of vector
void Vector_emplace_back(Vector* vector, void* data, bool owned);

// remove the last element from vector
void Vector_pop_back(Vector* vector);

// swap two elements
void Vector_swap(Vector* vector, size_t p, size_t q);

// get the element at specified location
void* Vector_at(Vector* vector, size_t index);

// apply function to each element within vector
//  do not manage ap in your function: foreach is in response for initializing and finalizing it for you
void Vector_foreach(Vector* vector, void(*function)(void* element, va_list ap), ...);
#endif