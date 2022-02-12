#ifndef _UMS_PROC_H_
#define _UMS_PROC_H_

/**
 * @brief Create the directory 'ums' in /proc and initialize the related structures
 */
void create_proc_ums(void);

/**
 * @brief Create the directory '<pid>' in /proc/ums/ and the directory 'schedulers' in /proc/ums/<pid>
 * 
 * In the description '<pid>' is the pid of the process who initialize the ums system
 */
void create_proc_ums_pid(void);

/**
 * @brief Create the dir '<sched_id>' in /proc/ums/<pid>/schedulers and all the files associated with it
 * 
 * The '<sched_id>' refers to the identifier of the scheduler that is assigned to each scheduler
 *  after calling this function 
 * @param sched_pid The pid of the scheduler 
 * @param parent_pid The pid of the process who initialize the ums system
 * @return The scheduler identifier assigned
 */
int create_scheduler_folder(int sched_pid, int parent_pid);

/**
 * @brief Create the file '<worker_id>' in /proc/ums/<pid>/schedulers/<sched_id>
 *
 * The '<worker_id>' refers to the identifier of the worker that is assigned to each worker
 *  after calling this function
 * @param wt_pid The pid of the worker
 * @param sched_id The scheduler identifier returned by create_scheduler_folder
 * @param parent_pid The pid of the process who initialize the ums system 
 * @return The worker identifier assigned
 */
int create_workers_file(int wt_pid, int sched_id, int parent_pid);

/**
 * @brief Remove the directory 'ums'
 */
void remove_proc_ums(void);

/**
 * @brief Remove all the directories and all the files inside '<pid>' in /proc/ums/
 *  and all the data structures related to them
 */
void remove_proc_ums_pid(void);

#endif