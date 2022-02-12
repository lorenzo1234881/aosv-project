#ifndef _KCOMPLETION_LIST_H_
#define _KCOMPLETION_LIST_H_

#include "ums_interface.h"

#include <linux/list.h>
#include <linux/hashtable.h>
#include <linux/mutex.h>
#include <linux/wait.h>

/**
 * @brief The kernel space rappresentation of the completion list
 */
typedef struct
{
    completion_list_common clc;
    int threads_ready; /**< Number of the current ready threads in the completion list */
    int nthreads; /**< Total number of threads that can still be executed */
    wait_queue_head_t wt_ready_wq; /**< Wait queue for the schedulers waiting other worker threads to be available */
    DECLARE_HASHTABLE(wt_ready_queue, WT_HTABLE_BITS); /**< Hashtable storing pointers to kcompletion_list structures */
    struct mutex mutex;
    struct hlist_node hnode;

} kcompletion_list;

/**
 * @brief reference to a kworker_thread inside the kcompletion_list
 */
typedef struct
{
    int tid; /**< tid of the worker thread */
    struct hlist_node hnode;

} kcompletion_list_elem;

/**
 * @brief Increment the threads_ready counter for every kcompletion_list associated with @p param1
 * 
 * @param kwt Pointer to the kworker_thread structure
 * @param kcl kcompletion_list pointer needed as a loop cursor
 * @param cle completion_list_element pointer needed as a loop cursor
 * @param lpos The &struct list_head to use as a loop cursor
 */
#define for_each_inc_kcl_threads_ready(kwt, kcl, cle, lpos) \
    list_for_each(lpos, &kwt->clid_list)\
    {\
        cle = list_entry(lpos, clid_list_elem, list);\
        kcl = get_kcompletion_list(cle->clid);\
        mutex_lock(&kcl->mutex);\
        kcl->threads_ready++;\
        mutex_unlock(&kcl->mutex);\
    }

/**
 * @brief Decrement the threads_ready counter for every kcompletion_list associated with @p param1
 * 
 * @param kwt Pointer to the kworker_thread structure
 * @param kcl kcompletion_list pointer needed for temporary storage
 * @param cle completion_list_element pointer needed for temporary storage
 * @param lpos The &struct list_head to use as a loop cursor
 */
#define for_each_dec_kcl_threads_ready(kwt, kcl, cle, lpos) \
    list_for_each(lpos, &kwt->clid_list)\
    {\
        cle = list_entry(lpos, clid_list_elem, list);\
        kcl = get_kcompletion_list(cle->clid);\
        mutex_lock(&kcl->mutex);\
        kcl->threads_ready--;\
        mutex_unlock(&kcl->mutex);\
    }

#define wake_up_schedulers(kwt, kcl, cle, lpos) \
    list_for_each(lpos, &kwt->clid_list)\
    {\
        cle = list_entry(lpos, clid_list_elem, list);\
        kcl = get_kcompletion_list(cle->clid);\
        wake_up_interruptible_all(&kcl->wt_ready_wq);\
    }
 

/**
 * @brief Decrement the nthreads counter for every kcompletion_list associated with @p param1
 * 
 * It also free the kcompletion_list_element associated with the kworker_thread
 * @param kwt Pointer to the kworker_thread structure
 * @param kcl kcompletion_list pointer needed for temporary storage
 * @param cle completion_list_element pointer needed for temporary storage
 * @param lpos The &struct list_head to use as a loop cursor
 * @param kcle kcompletion_list_element pointer needed for temporary storage
 * @param htmp a &struct hlist_node used for temporary storage
 */
#define for_each_dec_kcl_nthreads(kwt, kcl, cle, lpos, kcle, htmp) \
    list_for_each(lpos, &kwt->clid_list)\
    {\
        cle = list_entry(lpos, clid_list_elem, list);\
        kcl = get_kcompletion_list(cle->clid);\
        mutex_lock(&kcl->mutex);\
        hash_for_each_possible_safe(kcl->wt_ready_queue, kcle, htmp, hnode, kwt->tid)\
        {\
            if(kcle->tid == kwt->tid) {\
                hash_del(&kcle->hnode);\
                kfree(kcle);\
            }\
        }\
        kcl->nthreads--;\
        mutex_unlock(&kcl->mutex);\
    }

/**
 * @brief Initialize the hashtable that stores pointers to kcompletion_list
 */
void init_kcl_htable(void);

/**
 * @brief Initialize and allocate the kcompletion_list structure
 * @param list The completion_list_common associated with the completion list to be initialized
 */
void init_completion_list(completion_list_common* list);

/**
 * @brief Add a kworker_thread to a kcompletion_list structure
 * @param wt The worker_thread_common related to worker thread to be added
 * @param clid The completion_list associated to the kcompletion_list
 */
void add_worker_thread(worker_thread_common* wt, int clid);

/**
 * @brief Fill wt_ready_queue with with the tid of the ready worker threads in the completion list
 * @param list The completion_list_common associated with the comepltion list 
 * @param wt_ready_queue An array that will stores the identifiers of the readt worker threads
 * @return Return the number of ready threads in wt_ready_queue
 */
int get_wt_ready_queue(completion_list_common* list, int* wt_ready_queue);

/**
 * @brief Destroy a kcompletion list, previously initialized
 * @param list The completion_list_common associated with the completion list to be destroyed
 */
void destroy_completion_list(completion_list_common* list);

/**
 * @brief Given a completion list identifier return the pointer to the related kcompletion_lsit
 * @param clid The completion list identifier 
 * @return The pointer to the kcompletion_list associated with the clid
 */
kcompletion_list* get_kcompletion_list(int clid);

/**
 * @brief Destroy the hashtable that stores pointers to kcompletion_list 
 */
void destroy_kcl_htable(void);

#endif