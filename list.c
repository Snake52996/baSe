#include "list.h"
// deleter used in case the list node owns the data stored within
static void ListNode_deleter_(void* target_){
    RAII_delete(((ListNode*)target_)->data);
}
ListNode* ListNode_create(void* data, bool owned){
    ListNode* list_node = (ListNode*)malloc(sizeof(ListNode));
    list_node->data = data;
    list_node->next = NULL;
    list_node->prev = NULL;
    if(owned) RAII_set_deleter(list_node, ListNode_deleter_);
    else RAII_set_dummy_deleter(list_node);
    return list_node;
}
void List_initialize(List* list){
    list->head.data = NULL;
    list->head.next = &(list->head);
    list->head.prev = &(list->head);
    list->size = 0;
    RAII_set_deleter(list, (void(*)(void*))List_clear);
}
List* List_create(){
    List* list = (List*)malloc(sizeof(List));
    List_initialize(list);
    return list;
}
void List_clear(List* list){
    ListNode* temp = NULL;
    ListNode* current = list->head.next;
    while(current != &(list->head)){
        temp = current->next;
        RAII_delete(current);
        current = temp;
    }
    List_initialize(list);
}
bool List_empty(const List* list){
    return list->head.next == &(list->head);
}
size_t List_size(const List* list){
    return list->size;
}
ListNode* List_emplace_front(List* list, ListNode* list_node){
    return List_emplace_after(list, list_node, &list->head);
}
ListNode* List_emplace_back(List* list, ListNode* list_node){
    return List_emplace_before(list, list_node, &list->head);
}
ListNode* List_emplace_after(List* list, ListNode* insert, ListNode* after){
    insert->next = after->next;
    after->next->prev = insert;
    after->next = insert;
    insert->prev = after;
    list->size += 1;
    return insert;
}
ListNode* List_emplace_before(List* list, ListNode* insert, ListNode* before){
    insert->prev = before->prev;
    before->prev = insert;
    insert->next = before;
    insert->prev->next = insert;
    list->size += 1;
    return insert;
}
void List_detach(List* list, ListNode* list_node){
    if(list->size == 0) return;
    list_node->prev->next = list_node->next;
    list_node->next->prev = list_node->prev;
    list_node->next = list_node->prev = NULL;
    list->size -= 1;
}
void List_erase(List* list, ListNode* list_node){
    List_detach(list, list_node);
    RAII_delete(list_node);
}
bool List_default_comparator(const void* data, const void* rhs){
    return data == rhs;
}
ListNode* List_find(List* list, const void* data, bool (*comparator)(const void* data, const void* rhs)){
    if(comparator == NULL) comparator = List_default_comparator;
    for(ListNode* node = list->head.next; node != &list->head; node = node->next){
        if(comparator(data, node->data)) return node;
    }
    return NULL;
}
void* ListNode_get(const ListNode* list_node){
    return list_node->data;
}