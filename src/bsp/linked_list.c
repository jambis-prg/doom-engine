#include "linked_list.h"
#include <stdlib.h>
#include "logger.h"

linked_list_t l_create_list()
{
    linked_list_t list = { 0 };
    list.head = (linked_list_node_t*) malloc(sizeof(linked_list_node_t));
    list.head->next = NULL;
    list.curr = list.tail = list.head;
    list.size = 0;        

    return list;
}

linked_list_t l_create_list_range(uint16_t start, uint16_t end)
{
    linked_list_t list = { 0 };
    list.head = (linked_list_node_t*) malloc(sizeof(linked_list_node_t));
    list.head->next = NULL;
    list.curr = list.tail = list.head;
    list.size = 0;        

    for (uint16_t i = start; i < end; i++)
        l_append(&list, i);
        
    return list;
}

void l_append(linked_list_t *l, uint16_t value)
{
    linked_list_node_t *tmp = (linked_list_node_t*) malloc(sizeof(linked_list_node_t));

    l->tail->next = tmp;
    l->tail = l->tail->next; 
    tmp->element = value;
    tmp->next = NULL;
    l->size++;
}

void l_erase(linked_list_t *l)
{
    if (l->curr->next == NULL) return;
    
    linked_list_node_t *tmp = l->curr->next;

    if (l->tail == l->curr->next)
        l->tail = l->curr;
    
    l->curr->next = l->curr->next->next; 
    
    free(tmp);
    l->size--;
}

void l_next(linked_list_t *l)
{
    if (l->curr != l->tail)
        l->curr = l->curr->next;
}

linked_list_t l_intersection(linked_list_t *l1, linked_list_t *l2)
{
    linked_list_t result = l_create_list();
    if (l1->size == 0 || l2->size == 0) return result; 

    while (l1->curr->next->element < l2->head->next->element)
    {
        l_next(l1);

        if (l1->curr == l1->tail)
            return result;
    }

    for (uint16_t i = 0; i < l2->size; i++)
    {
        uint16_t value = l1->curr->next->element;
        if (value == l2->curr->next->element)
        {
            l_append(&result, value);
            l_next(l1);

            if (l1->curr == l1->tail)
                break;
        }

        l_next(l2);
    }

    return result;
}

linked_list_t l_intersection_remove(linked_list_t *l1, linked_list_t *l2)
{
    linked_list_t result = l_create_list();
    if (l1->size == 0 || l2->size == 0) return result; 

    while (l1->curr->next->element < l2->head->next->element)
    {
        l_next(l1);

        if (l1->curr == l1->tail)
            return result;
    }

    for (uint16_t i = 0; i < l2->size; i++)
    {
        uint16_t value = l1->curr->next->element;
        if (value == l2->curr->next->element)
        {
            l_append(&result, value);
            l_erase(l1);

            if (l1->curr == l1->tail)
                break;
        }
        
        l_next(l2);
    }

    return result;
}

void l_delete_list(linked_list_t *l)
{
    while (l->head != NULL)
    {
        l->curr = l->head;
        l->head = l->head->next;
        free(l->curr);
    }

    l->size = 0;
}