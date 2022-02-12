#!/bin/bash

run_test()
{
    make

    start_time="$(date -u +%s.%N)"


    for i in $(seq 1 $1);

    do

    './example.elf' > /dev/null || { echo 'test failed' ; 'sudo rmmod ums.ko'; exit 1; }

    done;

    end_time="$(date -u +%s.%N)"

    elapsed="$(bc <<<"$end_time-$start_time")"
    echo "Total of $elapsed seconds elapsed for process"
}

test1()
{
    NUM_OF_RUNS=180
    export N_WORKER_THREADS=5
    export N_COMPLETION_LISTS=1
    export WT_WORKLOAD=1000000
    export NUM_YIELDS=2

    echo "\nSTART TEST1\n"

    run_test $NUM_OF_RUNS
}

test2()
{
    NUM_OF_RUNS=90
    export N_WORKER_THREADS=5
    export N_COMPLETION_LISTS=2
    export WT_WORKLOAD=100000000
    export NUM_YIELDS=2

    echo "\nSTART TEST2\n"

    run_test $NUM_OF_RUNS
}

test3()
{
    NUM_OF_RUNS=270
    export N_WORKER_THREADS=5
    export N_COMPLETION_LISTS=1
    export WT_WORKLOAD=100000
    export NUM_YIELDS=5

    echo "\nSTART TEST3\n"

    run_test $NUM_OF_RUNS
}

test4()
{
    NUM_OF_RUNS=1000
    export N_WORKER_THREADS=5
    export N_COMPLETION_LISTS=1
    export WT_WORKLOAD=0
    export NUM_YIELDS=1

    echo "\nSTART TEST3\n"

    run_test $NUM_OF_RUNS
}

while test $# -gt 0; do
    case "$1" in 
    -b|--build)
        cd ../lkm
        make
        cd ../example
        make -C ../lib
        shift
        ;;
    *)
        ;;
    esac
done

eval "sudo insmod ../lkm/ums.ko"

test1

test2

test3

test4

eval "sudo rmmod ../lkm/ums.ko"

echo "\nEND TESTS\n"