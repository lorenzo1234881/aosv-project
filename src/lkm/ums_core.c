#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>

#include <linux/init.h>
#include <linux/slab.h>

#include <linux/sched.h>

#include <linux/uaccess.h> 

#include <linux/sched/task.h>

#include "kworker_thread.h"
#include "kcompletion_list.h"
#include "ksched_thread.h"
#include "ums_proc.h"
#include "ums_interface.h"

#define DEVICE_NAME "umsdev"

MODULE_AUTHOR("Lorenzo Santangelo");
MODULE_DESCRIPTION("User Mode Scheduling");
MODULE_LICENSE("GPL");
MODULE_VERSION("0.1");

int init_module(void);
void cleanup_module(void);
static long ums_ioctl(struct file *file, unsigned int request, unsigned long data);


static struct file_operations fops = {
    .unlocked_ioctl = ums_ioctl};

static struct miscdevice mdev = {
    .minor = 0,
    .name = DEVICE_NAME,
    .mode = S_IALLUGO,
    .fops = &fops};

int ums_init(void)
{
    int ret;
    printk(KERN_DEBUG MODULE_NAME_LOG "init\n");

    ret = misc_register(&mdev);

    if (ret < 0)
    {
        printk(KERN_ALERT MODULE_NAME_LOG "Registering char device failed\n");
        return ret;
    }

    printk(KERN_DEBUG MODULE_NAME_LOG "Device registered successfully\n");

    init_kcl_htable();
    init_kwt_htable();
    init_kst_htable();
    create_proc_ums();

    return 0;
}

void ums_exit(void)
{
    printk(KERN_DEBUG MODULE_NAME_LOG "cleaning\n");

    misc_deregister(&mdev);

    destroy_kcl_htable();
    destroy_kwt_htable();
    destroy_kst_htable();

    remove_proc_ums();

    printk(KERN_DEBUG MODULE_NAME_LOG "exit\n");
}

static long ums_ioctl(struct file *file, unsigned int request, unsigned long data)
{
    completion_list_common list;

    struct pid *pid;
    struct task_struct *proc;
    
    worker_thread_common wt;
    kworker_thread *kwt;

    kcompletion_list *kcl;

    ksched_thread *kst;

    ktime_t elapsed_wt_running, current_time;

    dequeue_completion_list_args dcl_args;
    completion_list_add_args cla_args;
    create_worker_thread_args cwt_args;

    kcompletion_list_elem *kcle;
    clid_list_elem *cle;

    struct hlist_node *htmp;

    struct list_head *lpos;
    int bkt;
    int ret;

    switch(request)
    {
        case UMSINIT:
            create_proc_ums_pid();
            break;

        case UMSINITCOMPLLIST:
            copy_from_user(&list, (completion_list_common*) data, sizeof(completion_list_common));
            init_completion_list(&list);
            copy_to_user((completion_list_common*) data, &list, sizeof(completion_list_common));
            break;

        case ENTERUMSSCHEDMODE:
            copy_from_user(&list, (completion_list_common*) data, sizeof(completion_list_common));

            create_sched_thread(&list);

            kst = get_ksched_thread(current->pid);
            kcl = get_kcompletion_list(list.id);

            kst->proc_id = create_scheduler_folder(current->pid, current->group_leader->pid);

            hash_for_each_rcu(kcl->wt_ready_queue, bkt, kcle, hnode) {
                create_workers_file(kcle->tid, kst->proc_id, current->group_leader->pid);
            }

            break;

        case CREATEWORKERTHREAD:
            copy_from_user(&cwt_args, (create_worker_thread_args*) data, sizeof(create_worker_thread_args));

            create_worker_thread(&cwt_args.wtc);

            pid = find_get_pid(cwt_args.parent_tid);
            proc = get_pid_task(pid, PIDTYPE_PID);

            printk("pid=%d wake up proc=%d\n", current->pid, cwt_args.parent_tid);
            wake_up_process(proc);

            put_task_struct(proc);

            set_current_state(TASK_INTERRUPTIBLE);
            schedule();
            break;

        case WAITWTCREATION:
            copy_from_user(&wt, (worker_thread_common*) data, sizeof(worker_thread_common));
            kwt = get_kworker_thread(wt.tid);
            set_current_state(TASK_INTERRUPTIBLE);
            while (kwt ==  (kworker_thread*)-1)
            {
                schedule();
                set_current_state(TASK_INTERRUPTIBLE);
                printk("pid=%d wait wt.tid=%d\n", current->pid, wt.tid);
                kwt = get_kworker_thread(wt.tid);
            }
            printk("keep going\n");
            printk("kwt->tid=%d\n",kwt->tid);
             __set_current_state(TASK_RUNNING);
            break;

        case DEQUEUECOMPLLIST:
            copy_from_user(&dcl_args, (dequeue_completion_list_args*) data, sizeof(dequeue_completion_list_args));

            kcl = get_kcompletion_list(dcl_args.clc.id);

            dcl_args.n = 0;
            while(dcl_args.n == 0) {

                printk(KERN_DEBUG MODULE_NAME_LOG "pid=%d wait kcl->threads_ready=%d > 0 || kcl->nthreads=%d == 0\n", current->pid, kcl->threads_ready, kcl->nthreads);
                wait_event_interruptible(kcl->wt_ready_wq, kcl->threads_ready > 0 || kcl->nthreads == 0);
                dcl_args.n = get_wt_ready_queue(&dcl_args.clc, dcl_args.wt_ready_tid);

            }

            copy_to_user((dequeue_completion_list_args*) data, &dcl_args, sizeof(dequeue_completion_list_args));
            break;

        case COMPLLISTADD:
            copy_from_user(&cla_args, (completion_list_add_args*) data, sizeof(completion_list_add_args));

            worker_thread_add_clid(&cla_args.wtc, cla_args.clc.id);

            add_worker_thread(&cla_args.wtc, cla_args.clc.id);
            break;

        case EXECUTEUMSTHREAD:
            copy_from_user(&wt, (worker_thread_common*) data, sizeof(worker_thread_common));

            kwt = get_kworker_thread(wt.tid);

            kst = get_ksched_thread(current->pid);

            set_kst_in_running_state(kst, wt)

            set_kwt_in_running_state(kwt, kst->tid)

            printk(KERN_DEBUG MODULE_NAME_LOG "pid=%d execute tid=%d\n", current->pid, wt.tid);

            wake_up_worker(kwt, proc, pid, ret)

            printk(KERN_DEBUG MODULE_NAME_LOG "pid=%d wait tid=%d state=%d != %d\n", current->pid, kwt->tid, kwt->state, WT_RUNNING);

            wait_worker_thread(kwt)

            update_time_measured(kwt, elapsed_wt_running, current_time)

            set_kst_in_idle_state(kst, elapsed_wt_running)

            if(kwt->state == WT_IDLE) {
                for_each_inc_kcl_threads_ready(kwt, kcl, cle, lpos)
            }

            wake_up_schedulers(kwt, kcl, cle, lpos)

            printk(KERN_DEBUG MODULE_NAME_LOG "pid=%d unlock tid=%d\n", current->pid, wt.tid);
            mutex_unlock(&kwt->mutex);

            break;

        case UMSTHREADTYIELD:
            copy_from_user(&wt, (worker_thread_common*) data, sizeof(worker_thread_common));

            printk(KERN_DEBUG MODULE_NAME_LOG "pid=%d yield\n", current->pid);

            kwt = get_kworker_thread(wt.tid);

            set_kwt_in_idle_state(kwt)

            wake_up_scheduler(kwt, proc, pid, ret)

            set_current_state(TASK_INTERRUPTIBLE);

            schedule();
            break;

        case WORKERTHREADEXIT:
            copy_from_user(&wt, (worker_thread_common*) data, sizeof(worker_thread_common));

            printk(KERN_DEBUG MODULE_NAME_LOG "tid=%d exiting\n", wt.tid);

            kwt = get_kworker_thread(wt.tid);

            set_kwt_in_exiting_state(kwt)

            for_each_dec_kcl_nthreads(kwt, kcl, cle, lpos, kcle, htmp)

            wake_up_scheduler(kwt, proc, pid, ret)

            break;

        case COMPLLISTDESTROY:
            copy_from_user(&list, (completion_list_common*) data, sizeof(completion_list_common));
            destroy_completion_list(&list);
            break;

        case WORKERTHREADDESTROY:
            copy_from_user(&wt, (worker_thread_common*) data, sizeof(worker_thread_common));
            destroy_kworker_thread(&wt);
            break;

        case SCHEDTHREADDESTROY:
            destroy_ksched_thread(current->pid);
            break;

        case UMSDESTROY:
            remove_proc_ums_pid();
            break;

        default:
            return -EINVAL;
    }

    return 0;
}

module_init(ums_init);
module_exit(ums_exit);