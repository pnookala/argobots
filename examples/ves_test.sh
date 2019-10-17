#!/bin/bash

num_threads=(72) # 114 216 288)
cpus=$(nproc)
echo "Number of cores is $cpus"

summaryfilename=${hostname}-mm-exectime
for threads in "${num_threads[@]}"
    do
        for i in {1..4}
        do
            if [ ${threads} -le ${cpus} ]
            then
                echo "Benchmarking matrix multiplication with $threads thread(s)"
                time ./nested_abt $i $threads
                echo ""
            fi
        done
    done

#python3 lineplot.py 1 4 $summaryfilename $hostname "throughput"
#bash ves_test.sh 2>&1 | tee result
