#define _GNU_SOURCE
#include "ums.h"

#include <stdio.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <errno.h>

#include <sys/ioctl.h>

#include <unistd.h>
#include <sys/syscall.h>

#include <pthread.h>

typedef struct {
    worker_thread* wt;
    int parent_tid;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} ums_create_wt_aux_args;


static int file_descriptor;

void* EnterUmsSchedulingMode(void* args)
{
    enter_ums_mode_args* ua = (void*) args;

    sched_setaffinity(0, sizeof(cpu_set_t), &ua->cpu_set);

    if( ioctl(file_descriptor, ENTERUMSSCHEDMODE, &ua->list->clc) < 0 )
    {
        perror("completion_list_init");
        return (void*) -1;
    }

    ua->scheduler_proc(args);

    if( ioctl(file_descriptor, SCHEDTHREADDESTROY, 0) < 0 )
    {
        perror("completion_list_init");
        return (void*) -1;
    }

    return NULL;
}

int ExecuteUmsThread(worker_thread* wt, completion_list* list, cpu_set_t cpu_set)
{
    wt->cpu_set = cpu_set;

    if (ioctl(file_descriptor, EXECUTEUMSTHREAD, &wt->wtc) < 0)
    {
        perror("ExecuteUmsThread");
        return -1;
    }

    return 0;
}

int UmsThreadYield(worker_thread* self)
{
    if (ioctl(file_descriptor, UMSTHREADTYIELD, &self->wtc) < 0)
    {
        perror("UmsThreadYield");
        return -1;
    }

    sched_setaffinity(0, sizeof(cpu_set_t), &self->cpu_set);

    return 0;
}

int DequeueUmsCompletionListItems(dequeue_completion_list_args* dcl_args, completion_list* list)
{
    dcl_args->clc.id = list->clc.id;

    if (ioctl(file_descriptor, DEQUEUECOMPLLIST, dcl_args) < 0)
    {
        perror("DequeueUmsCompletionListItems");
        return -1;
    }

    return 0;
}

int ums_init(void)
{
    file_descriptor = open("/dev/umsdev", 0);

    if(file_descriptor < 0) {
        perror("ums_open_device");
        return -1;
    }

    if( ioctl(file_descriptor, UMSINIT, 0) < 0 )
    {
        perror("completion_list_init");
        return -1;
    }

    return 0;
}

int ums_destroy(void)
{
    if( ioctl(file_descriptor, UMSDESTROY, 0) < 0 )
    {
        perror("completion_list_init");
        return -1;
    }

    return close(file_descriptor);
}

int ums_completion_list_init(completion_list* list)
{
    completion_list_init(list);

    if( ioctl(file_descriptor, UMSINITCOMPLLIST, &list->clc) < 0 )
    {
        perror("completion_list_init");
        return -1;
    }

    return 0;
}

int ums_completion_list_add(completion_list* list, worker_thread* wt)
{
    completion_list_add_args claa;

    completion_list_add(list, wt);

    claa.clc = list->clc;
    claa.wtc = wt->wtc;

    if( ioctl(file_descriptor, COMPLLISTADD, &claa) < 0)
    {
        perror("completion_list_add");
        return -1;
    }

    return 0;
}

static void* ums_create_worker_thread_aux(void* arg)
{
    create_worker_thread_args cwt_args;
    ums_create_wt_aux_args *ucwa_args;
    worker_thread* wt;

    ucwa_args = (ums_create_wt_aux_args*) arg;
    wt = ucwa_args->wt;
    cwt_args.parent_tid = ucwa_args->parent_tid;

    pid_t tid = syscall(SYS_gettid);

    pthread_mutex_lock(&ucwa_args->mutex);

    printf("setting wt tid\n");
    worker_thread_set_tid(wt, tid);

    pthread_cond_signal(&ucwa_args->cond);
    pthread_mutex_unlock(&ucwa_args->mutex);

    cwt_args.wtc = wt->wtc;

    if( ioctl(file_descriptor, CREATEWORKERTHREAD, &cwt_args) < 0 )
    {
        perror("create_worker_thread_aux");
        return (void*) -1;
    }

    sched_setaffinity(0, sizeof(cpu_set_t), &wt->cpu_set);

    wt->worker_routine(wt);

    if( ioctl(file_descriptor, WORKERTHREADEXIT, &wt->wtc) < 0 )
    {
        perror("create_worker_thread_aux");
        return (void*) -1;
    }

    return 0;
}

int ums_create_worker_thread(worker_thread* wt, void *(*worker_routine) (void *))
{
    pthread_t t;
    ums_create_wt_aux_args ucwa_args;

    ucwa_args.parent_tid = syscall(SYS_gettid);
    ucwa_args.wt = wt;

    worker_thread_init(wt, worker_routine);

    if (pthread_cond_init(&ucwa_args.cond, NULL) != 0) {
        perror("ums_create_worker_thread - pthread_cond_init");
        return -1;
    }

    if(pthread_mutex_init(&ucwa_args.mutex, NULL) != 0) {
        perror("ums_create_worker_thread - pthread_mutex_init");
        return -1;
    }

    pthread_create(&t, NULL, &ums_create_worker_thread_aux, &ucwa_args);

    pthread_mutex_lock(&ucwa_args.mutex);
    while (ucwa_args.wt->wtc.tid == -1) {
        printf("waiting for ucwa_args.wt->wtc.tid=%d != -1\n", ucwa_args.wt->wtc.tid);
        pthread_cond_wait(&ucwa_args.cond, &ucwa_args.mutex);
    }
    pthread_mutex_unlock(&ucwa_args.mutex);

    pthread_cond_destroy(&ucwa_args.cond);

    if( ioctl(file_descriptor, WAITWTCREATION, &wt->wtc) < 0 )
    {
        perror("wait_wt_creation");
        return -1;
    }

    return 0;
}

int ums_completion_list_destroy(completion_list* list)
{
    completion_list_destroy(list);

    if( ioctl(file_descriptor, COMPLLISTDESTROY, &list->clc) < 0 )
    {
        perror("ums_completion_list_destroy");
        return -1;
    }

    return 0;
}

int ums_worker_thread_destroy(worker_thread* wt)
{
    worker_thread_destroy(wt);

    if( ioctl(file_descriptor, WORKERTHREADDESTROY, &wt->wtc) < 0 )
    {
        perror("ums_completion_list_destroy");
        return -1;
    }

    return 0;
}