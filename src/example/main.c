#define _GNU_SOURCE

#include <ums.h>

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/syscall.h>
#include <sched.h>

void* f(void* arg)
{
    worker_thread* self = (worker_thread*) arg;

    for(int i = 0; i < NUM_YIELDS; i++) {

        printf("executing thread %d\n", self->wtc.tid);

        for(unsigned i = 0; i < WT_WORKLOAD; i++);
        
        UmsThreadYield(self);
    }

    printf("exiting\n");

    return NULL;
}

void* scheduler_proc(void* arg)
{
    cpu_set_t cpu_set;

    dequeue_completion_list_args dcl_args;
    enter_ums_mode_args* ua = (enter_ums_mode_args*) arg;

    completion_list* list = ua->list;

    worker_thread* wt;

    do {
        DequeueUmsCompletionListItems(&dcl_args, list);

        sched_getaffinity(0, sizeof(cpu_set_t), &cpu_set);

        for(int i = 0; i < dcl_args.n; i++)
        {
            wt = completion_list_get(list, dcl_args.wt_ready_tid[i]);
            if(wt != (worker_thread*)-1)
                ExecuteUmsThread(wt, list, ua->cpu_set);
        }
    } while(dcl_args.n > 0);

    return NULL;
}

int main(int argc, char **argv)
{
    enter_ums_mode_args* ua;

    completion_list list[N_COMPLETION_LISTS];
    worker_thread wt[N_WORKER_THREADS];
    pthread_t *sched;

    cpu_set_t cpu_set;

    sched_getaffinity(0, sizeof(cpu_set_t), &cpu_set);

    int ncpu = CPU_COUNT(&cpu_set);
    if(ncpu < N_COMPLETION_LISTS)
    {
        printf("The number of processors available must be greater or equal to number of completion lists\n");
        return -1;
    }

    sched = malloc(sizeof(pthread_t)*ncpu);
    ua = malloc(sizeof(enter_ums_mode_args)*ncpu);

    ums_init();

    for(int i = 0; i < N_COMPLETION_LISTS; i++)
        ums_completion_list_init(&list[i]);

    for(int i = 0; i < N_WORKER_THREADS; i++)
    {
        ums_create_worker_thread(&wt[i], &f);
        for(int j = 0; j < N_COMPLETION_LISTS; j++)
        {
            ums_completion_list_add(&list[j], &wt[i]);
        }
    }

    float increment = N_COMPLETION_LISTS/ncpu;

    for(int cpu = 0; cpu < ncpu; cpu++)
    {
        if(CPU_ISSET(cpu, &cpu_set))
        {
            CPU_SET(cpu, &ua[cpu].cpu_set);
                        
            ua[cpu].scheduler_proc = &scheduler_proc;

            ua[cpu].list = &list[(int)(cpu * increment)];
            pthread_create(&sched[cpu], NULL, &EnterUmsSchedulingMode, &ua[cpu]);
        }
    }
 
    for(int cpu = 0; cpu < ncpu; cpu++)
        pthread_join(sched[cpu], NULL);

    for(int i = 0; i < N_COMPLETION_LISTS; i++)
        ums_completion_list_destroy(&list[i]);
    for(int i = 0; i < N_WORKER_THREADS; i++)
        ums_worker_thread_destroy(&wt[i]);
    
    ums_destroy();

    free(sched);
    free(ua);

    return 0;
}