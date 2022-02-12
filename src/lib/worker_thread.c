#include "worker_thread.h"

#include <stdio.h>

int worker_thread_init(worker_thread* wt, void *(*worker_routine) (void *))
{
    wt->wtc.tid = -1;
    wt->worker_routine = worker_routine;

    return 0;
}

int worker_thread_set_tid(worker_thread* wt, int tid)
{
    wt->wtc.tid = tid;

    return 0;
}

int worker_thread_destroy(worker_thread* wt)
{
    return 0;
}