#!/bin/bash 

test_type=$1
num_ess=(72)
num_threads=(72 144 216 288 576 1152 2592 5184 10368)
cpus=$(nproc)
test_name=(noop-basic) # barrier-abt-ves) #(noop-private-abt-ves barrier-abt-ves nested-noop-abt)
exec_name=(noop) #( barier_test) #(nested_abt barrier_test nested_abt)
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
               for i in {1..10}
                do
                    echo "[$i] Benchmarking ${type} with $ess ES(s) and ${threads} thread(s)"
                    ./${exec_name} $ess ${threads}  out/${rawfilename}.dat
                    echo ""
                done
            done
        done
    done
