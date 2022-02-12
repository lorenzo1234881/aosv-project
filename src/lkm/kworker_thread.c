#include <linux/slab.h>

#include "kworker_thread.h"

static DEFINE_HASHTABLE(wt_htable, WT_HTABLE_BITS);
static struct mutex wt_htable_mutex;

void init_kwt_htable(void)
{
    mutex_init(&wt_htable_mutex);   
}

void create_worker_thread(worker_thread_common* wt)
{
    kworker_thread *kwt = kmalloc(sizeof(kworker_thread), GFP_KERNEL);

    kwt->tid = wt->tid;
    kwt->state = WT_IDLE;
    kwt->nswitches = 0;
    kwt->total_measured = 0;
    mutex_init(&kwt->mutex);
    mutex_init(&kwt->proc_mutex);
    INIT_LIST_HEAD(&kwt->clid_list);
    kwt->curr_sched_tid = -1;

    mutex_lock(&wt_htable_mutex);

    hash_add_rcu(wt_htable, &kwt->hnode, kwt->tid);

    printk(KERN_DEBUG MODULE_NAME_LOG "tid=%d create_worker_thread()\n", kwt->tid);

    mutex_unlock(&wt_htable_mutex);
}

int worker_thread_add_clid(worker_thread_common* wt, int clid)
{
    clid_list_elem* cle;
    kworker_thread* kwt;

    kwt = get_kworker_thread(wt->tid);
    cle = kmalloc(sizeof(clid_list_elem), GFP_KERNEL);
    cle->clid = clid;

    mutex_lock(&kwt->mutex);

    list_add(&cle->list, &kwt->clid_list);

    mutex_unlock(&kwt->mutex);

    return 0;
}

kworker_thread* get_kworker_thread(int tid)
{
    kworker_thread *kwt;

    hash_for_each_possible_rcu(wt_htable, kwt, hnode, tid) {
        if(kwt->tid == tid)
            return kwt;
    }

    return (kworker_thread*) -1;
}

int kworker_thread_copy(kworker_thread* kwt_dst, int tid)
{
    kworker_thread* kwt = get_kworker_thread(tid);
    if(kwt == (kworker_thread*) -1)
    {
        return -1;
    }

    mutex_lock(&kwt->proc_mutex);
    kwt_dst->tid = kwt->tid;
    kwt_dst->nswitches = kwt->nswitches;
    kwt_dst->state = kwt->state;
    kwt_dst->last_measured = kwt->last_measured;
    kwt_dst->total_measured = kwt->total_measured;


    *kwt_dst = *kwt;
    mutex_unlock(&kwt->proc_mutex);

    return 0;
}

void destroy_kworker_thread(worker_thread_common* wt)
{
    clid_list_elem* cle;
    struct list_head *lpos, *ln;
    kworker_thread *kwt = get_kworker_thread(wt->tid);

    if(kwt != (kworker_thread*) -1 )
    {
        printk(KERN_DEBUG MODULE_NAME_LOG "clean tid %d\n", wt->tid);

        list_for_each_safe(lpos, ln, &kwt->clid_list)
        {
            cle = list_entry(lpos, clid_list_elem, list);
            list_del(&cle->list);
            kfree(cle);
        }

        hash_del(&kwt->hnode);
        kfree(kwt);
    }
}

void destroy_kwt_htable(void)
{
    kworker_thread *kwt;
    struct hlist_node *htmp;
    int bkt;

    hash_for_each_safe(wt_htable, bkt, htmp, kwt, hnode) {

        printk(KERN_DEBUG MODULE_NAME_LOG "clean tid %d\n", kwt->tid);

        hash_del(&kwt->hnode);
        kfree(kwt);

    }
}