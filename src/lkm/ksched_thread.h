#ifndef _KSCHED_THREAD_H_
#define _KSCHED_THREAD_H_

#include <linux/mutex.h>
#include <linux/wait.h>
#include <linux/ktime.h>

#include "ums_interface.h"


/**
 * @brief The kernel space rappresentation of the scheduler thread
 */
typedef struct {

    int tid; /**< The tid of the scheduler thread */
    int proc_id;
    bool running; /**< Current state of the scheduler */
    int nswitches; /**< Number of times the scheduler switched to a worker thread */
    worker_thread_common wt_running; /**< The worker_thread_common structure of the current worker thread in execution*/
    ktime_t elapsed_wt_running; /**< the time needed for the last worker thread switch */
    completion_list_common clc; /**< The completion_list_common structure related to the completion list associated with the scheduler*/
    struct hlist_node hnode;
    struct mutex mutex;

} ksched_thread;

/**
 * @brief Set state of the ksched_thread to running
 * @param kst Pointer to the kscheduler_thread structure
 * @param wtc The worker_thread_common structure of the worker thread that will be executed
 */
#define set_kst_in_running_state(kst, wtc) \
    mutex_lock(&kst->mutex);\
    kst->nswitches++;\
    kst->wt_running = wtc;\
    kst->running = 1;\
    mutex_unlock(&kst->mutex);

/**
 * @brief Set state of the ksched_thread to idle
 * @param kst Pointer to the kscheduler_thread structure
 * @param elapsed_wt_running The ktime_t structure already storing the time
 *  needed for the last worker thread switch
 */
#define set_kst_in_idle_state(kst, elapsed_wt_running) \
    mutex_lock(&kst->mutex);\
    kst->elapsed_wt_running = elapsed_wt_running;\
    kst->running = 0;\
    mutex_unlock(&kst->mutex);

/**
 * @brief Wait a worker thread to wither yield or finish the execution
 */
#define wait_worker_thread(kwt) \
    set_current_state(TASK_INTERRUPTIBLE);\
    while (kwt->state == WT_RUNNING) {\
            schedule();\
            set_current_state(TASK_INTERRUPTIBLE);\
    }\
    __set_current_state(TASK_RUNNING);

/**
 * @brief Wake up a worker waiting to be executed
 * @param kwt Pointer to kworker thread structure that have to be awekend
 * @param proc task_struct pointer for temporary storage
 * @param pid struct pid for temporary storage
 * @param ret The return value of wake_up_process
 */

#define wake_up_worker(kwt, proc, pid, ret) \
    pid = find_get_pid(kwt->tid);\
    proc = get_pid_task(pid, PIDTYPE_PID);\
    ret = wake_up_process(proc);\
    printk("tid=%d wake_up worker tid=%d returned %d\n", current->pid, kwt->tid, ret);\
    put_task_struct(proc);


/**
 * @brief Initialize the hashtables that stores pointers to ksched_thread
 */
void init_kst_htable(void);

/**
 * @brief Create a sched_thread structures
 */
void create_sched_thread(completion_list_common* clc);

/**
 * @brief Given a tid of a scheduler thread return the pointer to the related ksched_thread
 */
ksched_thread* get_ksched_thread(int tid);

int ksched_thread_copy(ksched_thread* kst_dst, int tid);

void destroy_ksched_thread(int tid);

/**
 * @brief Destroy the hashtable that stores pointers to ksched_thread
 */
void destroy_kst_htable(void);

#endif