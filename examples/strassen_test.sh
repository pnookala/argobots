#!/bin/bash

#Strassen's test
num_ess=(72) # (1 2 4 8 16 24 32 48 64 72)
num_threads=(1 2 4 8 12 16 24 32 48 64 72)
test_name=(strassen-nested-8192)
exec_name=(strassen_thread) #(nested_abt barrier_test)
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
        #rawfilename=${hostname}-${type}
        #touch out/${rawfilename}.dat
        for ess in "${num_ess[@]}"
        do
            rawfilename=${hostname}-${type}-${ess}-abt-ves
            touch out/${rawfilename}.dat
            for threads in "${num_threads[@]}"
            do
                for i in {1..1}
                do
                    echo "[$i] Benchmarking ${type} with $ess ES(s) and $threads thread(s)"
                    ./${exec_name} 8192 1 16 1024 $ess $threads out/${rawfilename}.dat
                    #./${exec_name} 2048 1 16 128 $ess $threads out/${rawfilename}.dat
                    echo ""
                done
            done
        done
    done
