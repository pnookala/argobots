#!/bin/bash

#arch=`dpkg --print-architecture`
#echo "Architecture is $arch"

cd "$(dirname "$0")"
ABT_DIR="/home/poornimans/pnookala-argobots/argobots"

function compile_abt() {
    cd "${ABT_DIR}"
    make
    make install
}

echo "Compiling Argobots..."
compile_abt
echo "Compiling benchmarks..."

cd "${ABT_DIR}/examples"
#clang -g -O3 nested_abt.c -lm -I/home/poornimans/argobots-install/include -L/home/poornimans/argobots-install/lib -labt -o nested_abt
clang -g -O3 barrier_test.c -lm -I/home/poornimans/argobots-install/include -L/home/poornimans/argobots-install/lib -labt -o barrier_test
clang -g -O3 noop.c -lm -I/home/poornimans/argobots-install/include -L/home/poornimans/argobots-install/lib -labt -o noop
clang -g -O3 strassen_thread.c -lm -I/home/poornimans/argobots-install/include -L/home/poornimans/argobots-install/lib -labt -o strassen_thread


num_ess=(72 144 216 288)
#num_threads=(72 144 216 288)
cpus=$(nproc)
test_name=(noop-private-abt-ves barrier-abt-ves)
exec_name=(noop barier_test) #(nested_abt barrier_test)
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
            #for threads in "${num_threads[@]}"
            #do
                for i in {1..10}
                do
                    echo "[$i] Benchmarking ${type} with $ess ES(s) and 10368 thread(s)"
                    ./${exec_name} $ess 10368 out/${rawfilename}.dat
                    echo ""
                done
            #done
        done
    done

#Strassen's test
num_ess=(2 4 6 8 12)

#num_threads=(72 144 216 288)
test_name=(strassen-abt-ves)
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
        rawfilename=${hostname}-${type}
        touch out/${rawfilename}.dat
        for ess in "${num_ess[@]}"
        do
            #for threads in "${num_threads[@]}"
            #do
                for i in {1..10}
                do
                    echo "[$i] Benchmarking ${type} with $ess ES(s) and 10368 thread(s)"
                    ./${exec_name} 2048 1 16 128 1 $ess out/${rawfilename}.dat
                    echo ""
                done
            #done
        done
    done
#python3 lineplot.py 1 4 $summaryfilename $hostname "throughput"
#bash ves_test.sh 2>&1 | tee result
