
#include "ch.h"
#include "linked_list.h"

#include <stdlib.h>
#include <string.h>


linked_list_t*
linked_list_new()
{
  linked_list_t* l = calloc(1, sizeof(linked_list_t));
  return l;
}

void
linked_list_prepend(linked_list_t* l, void* data)
{
  linked_list_node_t* node = calloc(1, sizeof(linked_list_node_t));
  node->data = data;

  node->next = l->head;
  node->prev = NULL;

  if (l->head != NULL)
    l->head->prev = node;

  l->head = node;

  if (l->tail == NULL)
    l->tail = node;
}

void
linked_list_append(linked_list_t* l, void* data)
{
  linked_list_node_t* node = calloc(1, sizeof(linked_list_node_t));
  node->data = data;

  node->next = NULL;
  node->prev = l->tail;

  if (l->tail != NULL)
    l->tail->next = node;

  l->tail = node;

  if (l->head == NULL)
    l->head = node;
}

void
linked_list_remove(linked_list_t* l, void* data)
{
  linked_list_node_t* node;
  for (node = l->head; node != NULL; node = node->next) {
    if (node->data == data) {
      if (node->prev != NULL)
        node->prev->next = node->next;
      else
        l->head = node->next;

      if (node->next != NULL)
        node->next->prev = node->prev;
      else
        l->tail = node->prev;

      free(node);
      break;
    }
  }
}
