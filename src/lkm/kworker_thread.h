#ifndef _KWORKER_THREAD_H_
#define _KWORKER_THREAD_H_

#include "ums_interface.h"

#include <linux/list.h>
#include <linux/hashtable.h>
#include <linux/mutex.h>
#include <linux/wait.h>

#define WT_IDLE 0
#define WT_RUNNING 1
#define WT_EXITING 2

/**
 * @brief The kernel space rappresentation of the worker thread
 */
typedef struct 
{
    int tid; /**< The tid of the worker thread*/
    int state; /**< Current state of the worker thread*/
    int nswitches; /**< number of times the worker thread passed the execution*/
    ktime_t last_measured;
    ktime_t total_measured; /**< total running time of the thread */
    struct hlist_node hnode;
    struct mutex mutex;
    struct mutex proc_mutex;
    struct list_head clid_list; /**< list of clid_list_elem, to have a way to access the completion lists containing the worker thread*/
    int curr_sched_tid; /**< The tid of the current scheduler thread that put to execution the worker thread */

} kworker_thread;

/**
 * @brief Reference to a completion list containing the worker thread
 */
typedef struct
{
    int clid; /**< identifier of the completion list */
    struct list_head list;

} clid_list_elem;

/**
 * @brief Set a kworker thread to running state
 * @param kwt Pointer to kworker thread structure
 * @param tid The tid of the current scheduler thread that put to execution the worker thread
 */
#define set_kwt_in_running_state(kwt, tid) \
    mutex_lock(&kwt->proc_mutex);\
    kwt->state = WT_RUNNING;\
    kwt->last_measured = ktime_get();\
    kwt->curr_sched_tid = tid;\
    mutex_unlock(&kwt->proc_mutex);


/**
 * @brief Set a kworker thread to idle state
 * @param kwt Pointer to kworker thread structure
 */
#define set_kwt_in_idle_state(kwt) \
    mutex_lock(&kwt->proc_mutex);\
    kwt->nswitches++;\
    kwt->state = WT_IDLE;\
    mutex_unlock(&kwt->proc_mutex);

/**
 * @brief Set a kworker thread to exiting state
 * @param kwt Pointer to kworker thread structure
 */
#define set_kwt_in_exiting_state(kwt) \
    mutex_lock(&kwt->proc_mutex);\
    kwt->state = WT_EXITING;\
    mutex_unlock(&kwt->proc_mutex);

/**
 * @brief Update the total running time of the thread and the time
 *  needed for the worker thread switch
 * @param kwt Pointer to kworker thread structure
 * @param elapsed_wt_running The ktime_t structure where the time needed for the worker thread to switch will be stored
 * @param current_time The ktime_t structure where the total running time will be stored
 */
#define update_time_measured(kwt, elapsed_wt_running, current_time) \
    mutex_lock(&kwt->proc_mutex);\
    current_time = ktime_get();\
    elapsed_wt_running = ktime_sub(current_time, kwt->last_measured);\
    kwt->total_measured = ktime_add(kwt->total_measured, elapsed_wt_running);\
    kwt->last_measured = current_time;\
    mutex_unlock(&kwt->proc_mutex);

/**
 * @brief Wakes up the scheduler waiting for the worker thread
 * @param kwt Pointer to kworker thread structure that awakens the scheduler
 * @param proc task_struct pointer for temporary storage
 * @param pid struct pid for temporary storage
 * @param ret The return value of wake_up_process
 */
#define wake_up_scheduler(kwt, proc, pid, ret) \
    pid = find_get_pid(kwt->curr_sched_tid);\
    proc = get_pid_task(pid, PIDTYPE_PID);\
    ret = wake_up_process(proc);\
    printk("tid=%d wake_up scheduler tid=%d returned %d\n", current->pid, kwt->curr_sched_tid, ret);\
    put_task_struct(proc);\
    kwt->curr_sched_tid = -1;

/**
 * @brief Initialize the hashtables that stores pointers to kworker_thread
 */
void init_kwt_htable(void);

void create_worker_thread(worker_thread_common* wt);

/**
 * @brief Add a completion_list_element to the kworker_thread
 */
int worker_thread_add_clid(worker_thread_common* wt, int clid);

/**
 * @brief Given a tid of a worker thread return the pointer to the related kworker_thread
 */
kworker_thread* get_kworker_thread(int tid);

int kworker_thread_copy(kworker_thread* kwt_dst, int tid);

void destroy_kworker_thread(worker_thread_common* wt);

/**
 * @brief Destroy the hashtable that stores pointers to kworker_thread
 */
void destroy_kwt_htable(void);

#endif