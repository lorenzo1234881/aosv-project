#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/uaccess.h>
#include <linux/string.h>
#include <linux/timekeeping.h>
#include <linux/time.h>

#include "ksched_thread.h"
#include "kworker_thread.h"
#include "ums_interface.h"
#include "ums_proc.h"

#include "ums_proc_p.h"

#define PROCFILESZ 300
#define BUFSIZE 21 // enough to store 64bit signed integer

static struct proc_dir_entry *proc_ums;

static struct {

    DECLARE_HASHTABLE(htable, PROC_HTABLE_BITS);
    struct mutex mutex;

} proc_ums_pids;


void create_proc_ums(void)
{
    proc_ums = proc_mkdir("ums", NULL);
    hash_init(proc_ums_pids.htable);
}

void create_proc_ums_pid(void)
{
    proc_ums_pid *pup = kmalloc(sizeof(proc_ums_pid), GFP_KERNEL);
    char buf[BUFSIZE];

    pup->last_sched_id = 0;
    mutex_init(&pup->mutex);

    sprintf(buf, "%d", current->pid);
    pup->proc_ums_pid = proc_mkdir(buf, proc_ums);

    hash_add_rcu(proc_ums_pids.htable, &pup->hnode, current->pid);

    pup->proc_ums_pid_schedulers = proc_mkdir("schedulers", pup->proc_ums_pid);

    hash_init(pup->schedulers_htable);
}

int create_scheduler_folder(int sched_pid, int parent_pid)
{
    proc_ums_pid *pup;
    scheduler *sched = kmalloc(sizeof(scheduler), GFP_KERNEL);
    int last_sched_id = 0;
    char buf[BUFSIZE];

    sched->last_wt_id = 0;

    hash_for_each_possible_rcu(proc_ums_pids.htable, pup, hnode, parent_pid) {

        mutex_lock(&pup->mutex);

        sprintf(buf, "%d", pup->last_sched_id);
        sched->scheduler = proc_mkdir(buf, pup->proc_ums_pid_schedulers);

        hash_add_rcu(pup->schedulers_htable, &sched->hnode, pup->last_sched_id);

        last_sched_id = pup->last_sched_id;

        pup->last_sched_id++;

        mutex_unlock(&pup->mutex);

        sched->scheduler_info = proc_create("info", 0444, sched->scheduler, &sched_info_ops);
        sched->workers = proc_mkdir("workers", sched->scheduler);
        sched->sched_pid = sched_pid;

        hash_init(sched->workers_htable);
    };

    return last_sched_id;
}

int create_workers_file(int wt_pid, int sched_id, int parent_pid)
{
    proc_ums_pid *pup;
    scheduler *sched;
    int last_wt_id = 0;
    worker *w = kmalloc(sizeof(worker), GFP_KERNEL);
    char buf[BUFSIZE];

    hash_for_each_possible_rcu(proc_ums_pids.htable, pup, hnode, parent_pid) {

        hash_for_each_possible_rcu(pup->schedulers_htable, sched, hnode, sched_id) {

            mutex_lock(&sched->mutex);

            sprintf(buf, "%d", sched->last_wt_id);
            w->worker = proc_create(buf, 0444, sched->workers, &worker_ops);

            hash_add_rcu(sched->workers_htable, &w->hnode, sched->last_wt_id);

            last_wt_id = sched->last_wt_id;

            sched->last_wt_id++;

            mutex_unlock(&sched->mutex);

            w->wt_pid = wt_pid;
        }

    }

    return last_wt_id;
}

static ssize_t sched_info_write(struct file *file, const char __user *ubuf, size_t count, loff_t *ppos)
{
    return -EINVAL;
}

static ssize_t sched_info_read(struct file *file, char __user *ubuf, size_t count, loff_t *ppos)
{
    unsigned long sched_id, parent_pid;
    scheduler *sched;
    ksched_thread kst;
    struct timespec ts;

    char buf[PROCFILESZ];
    int len = 0;

    if (*ppos > 0 || count < PROCFILESZ)
        return 0;

    get_ids_from_schedulers_dir(file, &sched_id, &parent_pid);

    sched = get_scheduler(sched_id, parent_pid);
    if(sched == (scheduler*)-1)
    {
        return 0;
    }

    if(ksched_thread_copy(&kst, sched->sched_pid) == -1)
    {
        return 0;
    }

    len += sprintf(buf, 
        "scheduler tid=%d\n"\
        "number of times the scheduler switched to a worker thread=%d\n"\
        "the completion list=%d\n", kst.tid, kst.nswitches, kst.clc.id);

    if(ktime_to_timespec_cond(kst.elapsed_wt_running, &ts))
        len += sprintf(buf+len, "the time needed for the last worker thread switch=%lds %ldns\n", ts.tv_sec, ts.tv_nsec);

    if(kst.running == 1) {
        len += sprintf(buf+len, "the current state=running\nworker thread running=%d\n", kst.wt_running.tid);
    } else {
        len += sprintf(buf+len, "the current state=idle\n");
    }

    if (copy_to_user(ubuf, buf, len))
        return -EFAULT;
    *ppos = len;

    return len;
}

static ssize_t worker_write(struct file *file, const char __user *ubuf, size_t count, loff_t *ppos)
{
    return -EINVAL;
}

static ssize_t worker_read(struct file *file, char __user *ubuf, size_t count, loff_t *ppos)
{

    unsigned long worker_id, sched_id, parent_pid;
    worker *w;
    kworker_thread kwt;
    ktime_t total_measured;
    struct timespec ts;

    char buf[PROCFILESZ];
    int len = 0;

    if (*ppos > 0 || count < PROCFILESZ)
        return 0;

    get_ids_from_workers_dir(file, &worker_id, &sched_id, &parent_pid);

    w = get_worker(worker_id, sched_id, parent_pid);
    if(w == (worker*) -1)
    {
        return 0;
    }

    if(kworker_thread_copy(&kwt, w->wt_pid) == -1)
    {
        return 0;
    }

    len += sprintf(buf+len, "tid=%d\n", kwt.tid);

    if(kwt.state == WT_RUNNING)
    {
        len += sprintf(buf+len, "current state=running\n");

        total_measured = ktime_add(ktime_sub(ktime_get(), kwt.last_measured), kwt.total_measured);

        if(ktime_to_timespec_cond(total_measured, &ts))
            len += sprintf(buf+len, "total running time of the worker thread=%lds %ldns\n", ts.tv_sec, ts.tv_nsec);
    } 
    else 
    {

        if(ktime_to_timespec_cond(kwt.total_measured, &ts))
            len += sprintf(buf+len, "total running time of the worker thread=%lds %ldns\n", ts.tv_sec, ts.tv_nsec);

        len += sprintf(buf+len, "current state=idle\n");
    }

    len += sprintf(buf+len, "number of switches=%d\n", kwt.nswitches);


    if (copy_to_user(ubuf, buf, len))
        return -EFAULT;
    *ppos = len;

    return len;
}

void remove_proc_ums(void)
{
    proc_remove(proc_ums);
}

void remove_proc_ums_pid(void)
{
    int bktpup, bktsched, bktw;
    struct hlist_node *htmppup, *htmpsched, *htmpw; 
    proc_ums_pid *pup;
    scheduler *sched;
    worker *w;

    hash_for_each_safe(proc_ums_pids.htable, bktpup, htmppup, pup, hnode) {    
        
        hash_for_each_safe(pup->schedulers_htable, bktsched, htmpsched, sched, hnode) {
                
                hash_for_each_safe(sched->workers_htable, bktw, htmpw, w, hnode) {
                
                    proc_remove(w->worker);
                    hash_del(&w->hnode);
                    kfree(w);
                }
            
            proc_remove(sched->scheduler);
            proc_remove(sched->scheduler_info);
            proc_remove(sched->workers);
            hash_del(&sched->hnode);
            kfree(sched);
        }

        proc_remove(pup->proc_ums_pid);
        proc_remove(pup->proc_ums_pid_schedulers);
        hash_del(&pup->hnode);
        kfree(pup);
    }

}


void get_ids_from_schedulers_dir(struct file* file, unsigned long *sched_id, unsigned long *parent_pid)
{
    char buf[BUFSIZE];
    struct dentry *d_sid, *d_ppid;

    d_sid = file->f_path.dentry->d_parent;
    d_ppid = d_sid->d_parent->d_parent;

    sprintf(buf, "%s", d_sid->d_name.name);
    kstrtoul(buf, 10, sched_id);
    sprintf(buf, "%s", d_ppid->d_name.name);
    kstrtoul(buf, 10, parent_pid);
}

void get_ids_from_workers_dir(struct file* file, unsigned long *worker_id, unsigned long *sched_id, unsigned long *parent_pid)
{
    char buf[BUFSIZE];
    struct dentry *d_wid, *d_sid, *d_ppid;

    d_wid = file->f_path.dentry;
    d_sid = d_wid->d_parent->d_parent;
    d_ppid = d_sid->d_parent->d_parent;

    sprintf(buf, "%s", d_wid->d_name.name);
    kstrtoul(buf, 10, worker_id);
    sprintf(buf, "%s", d_sid->d_name.name);
    kstrtoul(buf, 10, sched_id);
    sprintf(buf, "%s", d_ppid->d_name.name);
    kstrtoul(buf, 10, parent_pid);
}

scheduler* get_scheduler(int sched_id, int parent_pid)
{
    proc_ums_pid *pup;
    scheduler *sched;


    hash_for_each_possible_rcu(proc_ums_pids.htable, pup, hnode, parent_pid) {

        hash_for_each_possible_rcu(pup->schedulers_htable, sched, hnode, sched_id) {
            return sched;
        }
    }

    return (scheduler*) -1;

}

worker* get_worker(int wt_id, int sched_id, int parent_pid)
{
    proc_ums_pid *pup;
    scheduler *sched;
    worker *w;


    hash_for_each_possible_rcu(proc_ums_pids.htable, pup, hnode, parent_pid) {

        hash_for_each_possible_rcu(pup->schedulers_htable, sched, hnode, sched_id) {

            hash_for_each_possible_rcu(sched->workers_htable, w, hnode, wt_id) {
                return w;
            }
        }
    }

    return (worker*) -1;
}