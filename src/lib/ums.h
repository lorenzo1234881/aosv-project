/**
 * @file ums.h
 */

#ifndef _UMS_H_
#define _UMS_H_

#include <ums_interface.h>
#include "completion_list.h"

#include <sched.h>


/**
 * @brief Structure that it is passed to the EnterUmsSchedulingMode
 */
typedef struct {

    completion_list* list; /**< Pointer to the scheduler's completion_list structure */
    cpu_set_t cpu_set; /**< The cpu_set_t that tells which core is associated to the scheduler */
    void *(*scheduler_proc) (void *); /**< Pointer to function that is executed for determining the next thread to be scheduled */

} enter_ums_mode_args;

/**
 * @brief Converts a standard pthread in a UMS Scheduler thread
 * @param args Pointer to an enter_ums_mode_args structure initialized by the caller
 * @return 0 on success, -1 otherwise
 */
void* EnterUmsSchedulingMode(void* args);

/**
 * @brief Obtains a list of current available thread to be run
 * 
 * Called from the scheduler thread obtains a list of current available thread to be run, 
 * if no thread is available to be run the function blocks until a thread becomes available
 * @param dcl_args Pointer to a dequeue_completion_list_args structure initialized by the function,
 * the kernel module updates the structures with the worker_thread_common of the worker thread that are ready
 * @param list Pointer to the scheduler's completion_list structure
 * @return 0 in success, -1 othrewise
 */
int DequeueUmsCompletionListItems(dequeue_completion_list_args* dcl_args, completion_list* list);

/**
 * @brief Executes a worker thread, it must be called by a scheduler thread
 * @param wt The worker thread that it is going to be executed
 * @param list Pointer to the scheduler's completion_list structure
 * @param cpu_set The cpu_set_t associated to the scheduler
 * @return 0 in success, -1 othrewise
 */
int ExecuteUmsThread(worker_thread* wt, completion_list* list, cpu_set_t cpu_set);

/**
 * @brief Pass the execution to another worker thread
 * @param self Pointer to the worker_thread structure of the current worker thread
 * @return 0 in success, -1 othrewise
 */
int UmsThreadYield(worker_thread* self);

/**
 * @brief Initialize the ums system, it should be called before using the ums library
 * @return 0 in success, -1 othrewise
 */
int ums_init(void);

/**
 * @brief Clean the ums system, it should be called at the end, after using the ums library
 * @return 0 in success, -1 othrewise
 */
int ums_destroy(void);

/**
 * @brief Initialize a completion list in the ums system,
 * the completion_list structure needs to be allocated by the caller
 * @param list Pointer to the completion_list that will be initialized
 * @return 0 in success, -1 othrewise
 */
int ums_completion_list_init(completion_list* list);

/**
 * @brief Associate a worker thread to a completion list
 * @param list Pointer to the completion_list
 * @param wt Pointer to the worker thread
 * @return 0 in success, -1 othrewise
 */
int ums_completion_list_add(completion_list* list, worker_thread* wt);

/**
 * @brief Create a worker thread in the ums system, the worker thread structure
 *  needs to be allocated by the caller
 * @param wt Pointer to the worker_thread
 * @param worker_routine the function that will be called by the worker thread
 * @return 0 in success, -1 othrewise
 */
int ums_create_worker_thread(worker_thread* wt, void *(*worker_routine) (void *));

/**
 * @brief Destroy a completion list, previously initialized, in the ums system
 * @return 0 in success, -1 othrewise
 */
int ums_completion_list_destroy(completion_list* list);

/**
 * @brief Destroy a worker thread, previously created, in the ums system
 * @return 0 in success, -1 othrewise
 */
int ums_worker_thread_destroy(worker_thread* wt);

#endif