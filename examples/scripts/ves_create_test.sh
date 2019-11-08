#!/bin/bash

test_type=$1
num_ess=(2 4 8 16 24 36 72)
cpus=$(nproc)
test_name=(xstream-create) #(noop-basic) # barrier-abt-ves) #(noop-private-abt-ves barrier-abt-ves nested-noop-abt)
exec_name=(ves_create) #( barier_test) #(nested_abt barrier_test nested_abt)
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
               for i in {1..50}
               do
                    echo "[$i] Benchmarking ${type} with $ess ES(s) thread(s)"
                    ./${exec_name} $ess out/${rawfilename}.dat
                    echo ""
               done
        done
    done
