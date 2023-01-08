#ifndef CxKANOAXDP_RAII_H_
#define CxKANOAXDP_RAII_H_
#include <stdlib.h>
#ifndef NDEBUG
#include <stdio.h>
#endif
// the RAII helper which should be embedded to any object to be saved in a container
//  use the custom deleter to perform destructions like deallocating spaces owned by
//  the object dynamicly allocated in the past, so that when the container clears
//  such object, the deallocating procedure can be carried out automaticaly
typedef struct{
    void (*deleter)(void* target_);
}RAII;
// set custom deleter for a RAII embedded object
void RAII_set_deleter(void* target_, void(*deleter)(void* target_));

// set custom deleter of a RAII embedded object to the dummy deleter
//  this function makes it easy for developers to do so, allowing
//  specify explicitly that there is no need for a custom deleter
//  maybe be caused by the lack of ownership
void RAII_set_dummy_deleter(void* target_);

// delete a RAII embedded object by first calling its custom deleter
//  and then free the space this object itself occupies
// Generate a warning in debugging mode for RAII embedded objects whose
//  deleter is not set
void RAII_delete(void* target_);
#endif