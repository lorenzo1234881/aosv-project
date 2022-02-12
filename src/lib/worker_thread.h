#ifndef _WORKER_THREAD_H_
#define _WORKER_THREAD_H_

#include <ums_interface.h>

#include <pthread.h>
#include <sched.h>

/**
 * @brief The user space rappresentation of the worker_thread
 */
typedef struct {
    worker_thread_common wtc;
    cpu_set_t cpu_set; /**< tells which core the worker_thread will run **/
    void *(*worker_routine) (void *);
} worker_thread;

int worker_thread_init(worker_thread* wt, void *(*worker_routine) (void *));
int worker_thread_set_tid(worker_thread* wt, int tid);
int worker_thread_destroy(worker_thread* wt);

#endif