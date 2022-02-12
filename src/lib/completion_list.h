#ifndef _COMPLETION_LIST_H_
#define _COMPLETION_LIST_H_

#include <ums_interface.h>

#include <pthread.h>

#include "worker_thread.h"

/**
 * @brief The user space rappresentation of the completion list
 */
typedef struct {

    int last_index; /**< last index in the wt array occupied by a worker_thread potiner */
    completion_list_common clc; /**< completion_list_common structure related to the compeltion_lsit */
    worker_thread* wt[MAX_WT]; /**< array containing pointers to worker_thread related to the worker threads contained in the completion list */

} completion_list;

int completion_list_init(completion_list* list);
int completion_list_add(completion_list* list, worker_thread* wt);
worker_thread* completion_list_get(completion_list* list, int tid);
int completion_list_destroy(completion_list* list);

#endif