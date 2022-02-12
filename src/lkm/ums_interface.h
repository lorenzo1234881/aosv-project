/**
 * @file ums_interface.h
 * @brief Contains data structures and macros in common between user space and kernel space
 */

#ifndef _UMS_INTERFACE_H_
#define _UMS_INTERFACE_H_

#define WT_HTABLE_BITS 3
#define CL_HTABLE_BITS 3
#define ST_HTABLE_BITS 3

#define PROC_HTABLE_BITS 3
#define PROC_ST_HTABLE_BITS 3
#define PROC_WT_HTABLE_BITS 3

#define MAX_WT 1<<WT_HTABLE_BITS
#define MAX_COMPL_LIST 1<<CL_HTABLE_BITS

#define MODULE_NAME_LOG "ums: "

/**
 * @brief 
 */
typedef struct {
    int tid; /**< tid of the worker thread */
} worker_thread_common;

/**
 * @brief
 */
typedef struct {
    int id; /**< The identifier of the completion list */
} completion_list_common;

/**
 * @brief The structure contains the arguments needed to dequeue a completion list
 * 
 * The clc field is initialized in the user space, and then it is passed to the kernel module via
 * ioctl with DEQUEUECOMPLLIST as request. The kernel module will update the wt_ready_tid with the
 * tid of the worker threads that are ready
 */
typedef struct {
    completion_list_common clc; /**< The completion_list_common structure of the completion list*/
    int n; /**< Numbers of ready worker threads*/
    int wt_ready_tid[MAX_WT]; /**< Array containing the tid of the ready worker threads*/
} dequeue_completion_list_args;

/**
 * @brief The structure contains the arguments needed to add a worker thread to a completion lsit
 * 
 * The structure is initialized in the user space and passed to the kernel module via ioctl
 * with COMPLLISTADD as request
 */
typedef struct
{
    completion_list_common clc; /**< The completion_list_common structure of the completion list */
    worker_thread_common wtc; /**< The worker_thread_common structure of the worker thread to add */
} completion_list_add_args;

typedef struct
{
    int parent_tid;
    worker_thread_common wtc;
} create_worker_thread_args;

#define UMSINITCOMPLLIST _IOWR(0x8C, 0x1, completion_list_common*)
#define CREATEWORKERTHREAD _IOW(0x8C, 0x2, worker_thread_common*)
#define EXECUTEUMSTHREAD _IOW(0x8c, 0x3, worker_thread_common*)
#define UMSTHREADTYIELD _IOW(0x8c, 0x4, worker_thread_common*)
#define COMPLLISTADD _IOW(0x8c, 0x5, completion_list_add_args*)
#define DEQUEUECOMPLLIST _IOWR(0x8c, 0x6, dequeue_completion_list_args*)
#define COMPLLISTDESTROY _IOW(0x8c, 0x7, completion_list_common*)
#define WORKERTHREADDESTROY _IOW(0x8c, 0x8, worker_thread_common*)
#define WORKERTHREADEXIT _IOW(0x8C, 0x9, worker_thread_common*)
#define WAITWTCREATION _IOW(0x8C, 0xA, worker_thread_common*)
#define ENTERUMSSCHEDMODE _IOW(0x8c, 0xB, completion_list_common*)
#define UMSINIT _IO(0x8c, 0xC)
#define UMSDESTROY _IO(0x8c, 0xD)
#define SCHEDTHREADDESTROY _IO(0x8c, 0xE)

#endif