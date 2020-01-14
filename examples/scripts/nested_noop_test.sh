#!/bin/bash 

test_type=$1
num_ess=(1) #(1 2 3 4)
num_threads=(72) # 144 288 576 1152 2592 5184) # 10368) # 41472 72000)
num_iterations=(1 100 1000 10000 100000 1000000) #10000000)
cpus=$(nproc)
test_name=(varying-noop) #(noop-basic mm-basic) # barrier-abt-ves) #(noop-private-abt-ves barrier-abt-ves nested-noop-abt)
exec_name=(nested_abt) #( barier_test matrixmul) #(nested_abt barrier_test nested_abt)
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
        rawfilename=${hostname}-${type}-${test_type}
        touch out/${rawfilename}.dat
        for ess in "${num_ess[@]}"
        do
            for threads in "${num_threads[@]}"
            do
               for i in "${num_iterations[@]}" #{1..100}
                do
                    echo "[$i] Benchmarking ${type} with $ess ES(s), ${threads} thread(s) and $i NOOPs"
                    ./${exec_name} $ess ${threads} $i out/${rawfilename}.dat
                    echo ""
                done
            done
        done
    done
