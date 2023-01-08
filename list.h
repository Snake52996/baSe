#ifndef CxKANOAXDP_List_H_
#define CxKANOAXDP_List_H_
#include "RAII.h"
#include <stdbool.h>
// singly linked loop list with head node
typedef struct ListNode_{
    RAII _;
    void* data;
    struct ListNode_* next;
    struct ListNode_* prev;
}ListNode;
typedef struct List_{
    RAII _;
    ListNode head;
    size_t size;
}List;

// create a new node to hold the data
//  use parameter owned to specify if this node is supposed to manage the data stored, which is generally
//  about deallocating it when the node itself is deallocated
ListNode* ListNode_create(void* data, bool owned);

// get data from ListNode
inline void* ListNode_get(const ListNode* list_node);

// initialize a list, setting up pointer, deleter and counter
void List_initialize(List* list);

// allocate memory for a list and initialize it
List* List_create();

// remove all elements in list, invoking deleter of each of them
void List_clear(List* list);

// check if a list is empty
inline bool List_empty(const List* list);

// get the size of a list
inline size_t List_size(const List* list);

// emplace a node before the first node in the list
//  return the list node emplaced
ListNode* List_emplace_front(List* list, ListNode* list_node);

// emplace a node after the last node in the list
//  return the list node emplaced
ListNode* List_emplace_back(List* list, ListNode* list_node);

// emplace a node after the given node in the list
//  return the list node emplaced
ListNode* List_emplace_after(List* list, ListNode* insert, ListNode* after);

// emplace a node before the given node in the list
//  return the list node emplaced
ListNode* List_emplace_before(List* list, ListNode* insert, ListNode* before);

// detach a list node from the list, do not modify the data fields
void List_detach(List* list, ListNode* list_node);

// erase a list node from the list, destroy the data fields with respect to its deleter
void List_erase(List* list, ListNode* list_node);

// default comparator which just apply ==, to simplify your code
bool List_default_comparator(const void* data, const void* rhs);

// find the first node in a list whose data that matches the one specified, return NULL if not found
ListNode* List_find(List* list, const void* data, bool (*comparator)(const void* data, const void* rhs));
#endif