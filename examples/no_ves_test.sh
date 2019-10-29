#!/bin/bash

cd "$(dirname "$0")"
ABT_DIR="/home/poornimans/argobots-src/argobots"

function compile_abt() {
    cd "${ABT_DIR}"
    make
    make install
}

echo "Compiling Argobots..."
compile_abt
echo "Compiling benchmarks..."

cd "/home/poornimans/pnookala-argobots/argobots/examples"
#clang -g -O3 nested_abt.c -lm -I/home/poornimans/argobots-install/include -L/home/poornimans/argobots-install/lib -labt -o nested_abt
clang -g -O3 barrier_test.c -lm -I/home/poornimans/argobots-install/include -L/home/poornimans/argobots-install/lib -labt -o barrier_test
clang -g -O3 noop.c -lm -I/home/poornimans/argobots-install/include -L/home/poornimans/argobots-install/lib -labt -o noop
clang -g -O3 strassen_thread.c -lm -I/home/poornimans/argobots-install/include -L/home/poornimans/argobots-install/lib -labt -o strassen_thread
clang -g -O3 nested_abt.c -lm -I/home/poornimans/argobots-install/include -L/home/poornimans/argobots-install/lib -labt -o nested_abt

num_ess=(1 2 3 4)  #(72 144 216 288)
#num_threads=(72 144 216 288)
cpus=$(nproc)
test_name=(nested-noop-abt-no-ves) #(noop-private-abt-no-ves barrier-abt-no-ves)
exec_name=(nested_abt) #(noop barier_test) #(nested_abt barrier_test)
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
                    ./${exec_name} $ess 72 out/${rawfilename}.dat
                    echo ""
                done
            #done
        done
    done

exit 1
#Strassen's test
num_ess=(72 144 216 288)

#Not running with error:
#[arch/abtd_stream.c:14] pthread_create
#[stream.c:309] ABTD_xstream_context_create
#[stream.c:321] ABTI_xstream_start: 29
#[stream.c:60] ABT_xstream_create: 29
#[stream.c:165] ABT_xstream_create_basic: 29
#[stream.c:274] ABT_xstream_start: 4

num_threads=(72 144 216 288)
#test_name=(strassen-top-level-abt-no-ves)
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
                for i in {1..5}
                do
                    echo "[$i] Benchmarking ${type} with $ess ES(s) and 10368 thread(s)"
                    ./${exec_name} 2048 1 16 128 $ess 1 out/${rawfilename}.dat
                    echo ""
                done
            #done
        done
    done
