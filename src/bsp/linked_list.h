#ifndef LINKED_LIST_H_INCLUDED
#define LINKED_LIST_H_INCLUDED

#include "typedefs.h"

typedef struct _linked_list_node
{
    uint16_t element;
    struct _linked_list_node *next;
} linked_list_node_t;

typedef struct _linked_list
{
    linked_list_node_t *head, *tail, *curr;
    uint16_t size;
} linked_list_t;

linked_list_t l_create_list();
linked_list_t l_create_list_range(uint16_t start, uint16_t end);

void l_append(linked_list_t *l, uint16_t value);
void l_erase(linked_list_t *l);
void l_next(linked_list_t *l);

linked_list_t l_intersection(linked_list_t *l1, linked_list_t *l2);
linked_list_t l_intersection_remove(linked_list_t *l1, linked_list_t *l2);

void l_delete_list(linked_list_t *l);

#endif