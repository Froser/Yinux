#pragma once
#include <yinux/types.h>

#define container_of(ptr,type,member)                                       \
({                                                                          \
    typeof(((type *)0)->member) * p = (ptr);                                \
    (type *)((unsigned long)p - (unsigned long)&(((type *)0)->member));     \
})

typedef struct List_t
{
    struct List_t * prev;
    struct List_t * next;
} List;

static void list_init(List* list)
{
    list->prev = list;
    list->next = list;
}

static List* list_next(List* entry)
{
    if(entry->next != NULL)
        return entry->next;
    else
        return NULL;
}

static void list_push_front(List* entry, List* newEntry)
{
    newEntry->next = entry;
    entry->prev->next = newEntry;
    newEntry->prev = entry->prev;
    entry->prev = newEntry;
}