#include "completion_list.h"

#include <stdio.h>

int completion_list_init(completion_list* list) 
{
    list->last_index = 0;

    return 0;
}

int completion_list_add(completion_list* list, worker_thread* wt)
{
    int last_index;

    last_index = list->last_index;

    if(last_index >= MAX_WT)
    {
        printf("completion list size exceeded\n");
        return -1;
    }

    list->wt[last_index] = wt;

    list->last_index++;

    return 0;
}

worker_thread* completion_list_get(completion_list* list, int tid)
{
    int n = list->last_index;

    for(int i = 0; i < n; i++)
    {
        if(list->wt[i]->wtc.tid == tid)
        {
            return list->wt[i];
        }
    }

    return (worker_thread*) -1;
}

int completion_list_destroy(completion_list* list)
{
    return 0;
}
