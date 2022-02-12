#include <linux/slab.h>
#include <linux/sched.h>

#include "kcompletion_list.h"
#include "kworker_thread.h"

static DEFINE_HASHTABLE(compl_list_htable, CL_HTABLE_BITS);
static struct mutex compl_list_htable_mutex;

static int compl_list_last_id = 0;

void init_kcl_htable(void)
{
    mutex_init(&compl_list_htable_mutex);
}

void init_completion_list(completion_list_common* list)
{
    kcompletion_list* kcl = kmalloc(sizeof(kcompletion_list), GFP_KERNEL);

    list->id = compl_list_last_id;

    kcl->clc.id = list->id;
    kcl->threads_ready = 0;
    kcl->nthreads = 0;

    mutex_init(&kcl->mutex);
    init_waitqueue_head(&kcl->wt_ready_wq);
    hash_init(kcl->wt_ready_queue);

    hash_add_rcu(compl_list_htable, &kcl->hnode, kcl->clc.id);

    mutex_lock(&compl_list_htable_mutex);
    printk(KERN_DEBUG MODULE_NAME_LOG "pid=%d assign clid=%d\n", current->pid, list->id);
    compl_list_last_id++;
    mutex_unlock(&compl_list_htable_mutex);
}

kcompletion_list* get_kcompletion_list(int clid)
{
    kcompletion_list *kcl;

    hash_for_each_possible_rcu(compl_list_htable, kcl, hnode, clid) {
        if(kcl->clc.id == clid)
            return kcl;
    }

    return (kcompletion_list*) -1;
}

void add_worker_thread(worker_thread_common* wt, int clid)
{
    kcompletion_list *kcl;
    kcompletion_list_elem *kcle = kmalloc(sizeof(kcompletion_list_elem), GFP_KERNEL);

    kcle->tid = wt->tid;

    kcl = get_kcompletion_list(clid);
    hash_add_rcu(kcl->wt_ready_queue, &kcle->hnode, kcle->tid);

    printk(KERN_DEBUG MODULE_NAME_LOG "pid=%d add_worker_thread() kcle->tid=%d in clid=%d\n", current->pid, kcle->tid, clid);

    kcl->threads_ready++;
    kcl->nthreads++;
}

int get_wt_ready_queue(completion_list_common* list, int* wt_ready_queue)
{
    kcompletion_list *kcl, *kcltmp;
    kcompletion_list_elem *kcle;
    kworker_thread *kwt;
    clid_list_elem *cle;

    struct list_head *lpos;
    int bkt;

    int i = 0;

    kcl = get_kcompletion_list(list->id);

    if(kcl->nthreads == 0)
        return -1;

    hash_for_each_rcu(kcl->wt_ready_queue, bkt, kcle, hnode) {
        kwt = get_kworker_thread(kcle->tid);
        if(i < MAX_WT) {
            if(mutex_trylock(&kwt->mutex))
            {
                wt_ready_queue[i++] =  kwt->tid;

                for_each_dec_kcl_threads_ready(kwt, kcltmp, cle, lpos)

                printk(KERN_DEBUG MODULE_NAME_LOG "pid=%d get_wt_ready_queue() lock tid=%d from clid=%d\n", current->pid, kwt->tid, list->id);
            }
            else
            {
                printk(KERN_DEBUG MODULE_NAME_LOG "pid=%d get_wt_ready_queue() can't acquire lock tid=%d from clid=%d\n", current->pid, kwt->tid, list->id);
            }
        }
    }

    return i;
}

void destroy_completion_list(completion_list_common* list)
{
    kcompletion_list *kcl;
    kcompletion_list_elem *kcle;
    struct hlist_node *htmp;
    int bkt;

    kcl = get_kcompletion_list(list->id);

    printk(KERN_DEBUG MODULE_NAME_LOG "clean clid=%d\n", kcl->clc.id);

    hash_for_each_safe(kcl->wt_ready_queue, bkt, htmp, kcle, hnode) {

        printk(KERN_DEBUG MODULE_NAME_LOG "clean kcle->tid=%d\n", kcle->tid);

        hash_del(&kcle->hnode);
        kfree(kcle);
    }

    hash_del(&kcl->hnode);
    kfree(kcl);
}

void destroy_kcl_htable(void)
{
    kcompletion_list *kcl;
    kcompletion_list_elem *kcle;
    struct hlist_node *htmpcl, *htmpwt;
    int bktcl, bktwt;

    hash_for_each_safe(compl_list_htable, bktcl, htmpcl, kcl, hnode){

        printk(KERN_DEBUG MODULE_NAME_LOG "clean clid=%d\n", kcl->clc.id);

        hash_for_each_safe(kcl->wt_ready_queue, bktwt, htmpwt, kcle, hnode) {

            printk(KERN_DEBUG MODULE_NAME_LOG "clean kcle->tid=%d\n", kcle->tid);

            hash_del(&kcle->hnode);
            kfree(kcle);
        }

        hash_del(&kcl->hnode);
        kfree(kcl);
    }    
}