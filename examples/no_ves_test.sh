#!/bin/bash

#arch=`dpkg --print-architecture`
#echo "Architecture is $arch"

num_ess=(72 144 216 288)
num_threads=(72 144 216 288)
cpus=$(nproc)
test_name=(barrier-abt-no-ves) #(mm-abt-no-ves)
hostname='haswell-72'
out_dir='out'
#$(awk '{print $1}' /etc/hostname)
echo "Number of cores is $cpus"

if [ -d "${out_dir}" ]; then
  echo "Output directory ${out_dir} already exists"
else
  mkdir ${out_dir}
fi

for type in "${test_name[@]}"
    do
        rawfilename=${hostname}-${type}
        touch out/${rawfilename}.dat
        for ess in "${num_ess[@]}"
        do
            for threads in "${num_threads[@]}"
            do
                for i in {1..10}
                do
                    echo "[$i] Benchmarking ${type} with $ess ES(s) and $threads thread(s)"
                    ./barrier_test $ess $threads out/${rawfilename}.dat
                    echo ""
                done
            done
        done
    done

#python3 lineplot.py 1 4 $summaryfilename $hostname "throughput"
#bash ves_test.sh 2>&1 | tee result
