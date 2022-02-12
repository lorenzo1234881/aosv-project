#ifndef _UMS_PROC_P_H_
#define _UMS_PROC_P_H_

#include <linux/hashtable.h>
#include <linux/mutex.h>
#include <linux/proc_fs.h>

/**
 * @brief This structure contains all the variables
 *  and structures related to the '<pid>' directory in /proc/ums
 */
typedef struct
{
    struct proc_dir_entry *proc_ums_pid; /**< The pointer to the struct proc_dir_entry of the '<pid>' directory */
    struct proc_dir_entry *proc_ums_pid_schedulers; /**< The pointer to the struct proc_dir_entry of the 'schedulers' directory */
    int last_sched_id; /**< Store the last scheduler identifer assigned */
    struct hlist_node hnode;
    struct mutex mutex;
    DECLARE_HASHTABLE(schedulers_htable, PROC_ST_HTABLE_BITS); /**< Hashtable that contains pointers to scheduler structures */

} proc_ums_pid;

/**
 * @brief This structure contains all the variables and structures related
 *  to the '<sched_id>' directory in /proc/ums/'<pid>'/schedulers
 */
typedef struct
{
    struct proc_dir_entry *scheduler; /**< The pointer to the struct proc_dir_entry of the '<sched_id>' directory */
    struct proc_dir_entry *scheduler_info; /**< The pointer to the struct proc_dir_entry of the 'info' file */
    struct proc_dir_entry *workers; /**< The pointer to the struct proc_dir_entry of the 'workers' directory */
    int sched_pid; /**< Store the pid of the scheduler */
    int last_wt_id; /**< Store the last worker identifer assigned */
    struct hlist_node hnode;
    struct mutex mutex;
    DECLARE_HASHTABLE(workers_htable, PROC_WT_HTABLE_BITS); /**< Hashtable that contains pointers to workers structures */

} scheduler;

/**
 * @brief This structure contains all the bariables and structures related
 *  to the '<worker_id>' file in /proc/ums/'<pid>'/schedulers/'<sched_id>'/workers
 */
typedef struct
{
    struct proc_dir_entry *worker; /**< The pointer to the struct proc_dir_entry of the '<worker_id>' file */
    int wt_pid; /**< Store the pid of the worker */
    struct hlist_node hnode;
    struct mutex mutex;

} worker;

scheduler* get_scheduler(int sched_id, int parent_pid);
worker* get_worker(int wt_id, int sched_id, int parent_pid);

/**
 * @brief Given a pointer to struct file of the 'info' file returns the related scheduler identifer and parent pid
 */
void get_ids_from_schedulers_dir(struct file* file, unsigned long *sched_id, unsigned long *parent_pid);

/**
 * @brief Given a pointer to struct file of the '<worker_id>' file returns the related worker identifer, scheduler identifier and parent pid
 */
void get_ids_from_workers_dir(struct file* file, unsigned long *worker_id, unsigned long *sched_id, unsigned long *parent_pid);

static ssize_t sched_info_read(struct file *file, char __user *ubuf, size_t count, loff_t *ppos);
static ssize_t sched_info_write(struct file *file, const char __user *ubuf, size_t count, loff_t *ppos);

static ssize_t worker_read(struct file *file, char __user *ubuf, size_t count, loff_t *ppos);
static ssize_t worker_write(struct file *file, const char __user *ubuf, size_t count, loff_t *ppos);

static struct file_operations sched_info_ops = {
    .read = sched_info_read,
    .write = sched_info_write
};

static struct file_operations worker_ops = {
    .read = worker_read,
    .write = worker_write
};

#endif