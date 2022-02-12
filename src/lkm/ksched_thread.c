#include <linux/slab.h>
#include <linux/hashtable.h>
#include <linux/sched.h>

#include "ksched_thread.h"

static DEFINE_HASHTABLE(st_htable, ST_HTABLE_BITS);
static struct mutex st_htable_mutex;

void init_kst_htable(void)
{
    mutex_init(&st_htable_mutex);
}

void create_sched_thread(completion_list_common* clc)
{
    ksched_thread *kst = kmalloc(sizeof(ksched_thread), GFP_KERNEL);

    kst->tid = current->pid;
    kst->running = 0;
    kst->elapsed_wt_running = 0;
    kst->nswitches = 0;
    kst->clc.id = clc->id;

    mutex_init(&kst->mutex);

    mutex_lock(&st_htable_mutex);

    hash_add_rcu(st_htable, &kst->hnode, kst->tid);

    printk(KERN_DEBUG MODULE_NAME_LOG "tid=%d create_sched_thread()\n", kst->tid);

    mutex_unlock(&st_htable_mutex);
}

ksched_thread* get_ksched_thread(int tid)
{
    ksched_thread *kst;

    hash_for_each_possible_rcu(st_htable, kst, hnode, tid) {
        if(kst->tid == tid)
            return kst;
    }

    return (ksched_thread*) -1;
}

int ksched_thread_copy(ksched_thread* kst_dst, int tid)
{
    ksched_thread *kst = get_ksched_thread(tid);
    if(kst == (ksched_thread*) -1)
    {
        return -1;
    }

    mutex_lock(&kst->mutex);
    *kst_dst = *kst;
    mutex_unlock(&kst->mutex);

    return 0;
}

void destroy_ksched_thread(int tid)
{
    ksched_thread *kst = get_ksched_thread(tid);
    printk(KERN_DEBUG MODULE_NAME_LOG "clean scheduler tid=%d\n", kst->tid);
    hash_del(&kst->hnode);
    kfree(kst);
}


void destroy_kst_htable(void)
{
    ksched_thread *kst;
    struct hlist_node *htmp;
    int bkt;

    hash_for_each_safe(st_htable, bkt, htmp, kst, hnode) {

        printk(KERN_DEBUG MODULE_NAME_LOG "clean scheduler tid=%d\n", kst->tid);

        hash_del(&kst->hnode);
        kfree(kst);

    }    
}