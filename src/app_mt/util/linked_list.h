
#ifndef LINKED_LIST_H
#define LINKED_LIST_H


typedef struct linked_list_node_s {
  void* data;
  struct linked_list_node_s* next;
  struct linked_list_node_s* prev;
} linked_list_node_t;

typedef struct {
  linked_list_node_t* head;
  linked_list_node_t* tail;
} linked_list_t;


linked_list_t*
linked_list_new(void);

void
linked_list_prepend(linked_list_t* list, void* data);

void
linked_list_append(linked_list_t* list, void* data);

void
linked_list_remove(linked_list_t* list, void* data);

#endif
