#!/bin/bash

#arch=`dpkg --print-architecture`
#echo "Architecture is $arch"

cd "$(dirname "$0")"
test_type=$1

if [ ${test_type} = "abt-ves" ] ; then
    ABT_DIR="/home/poornimans/pnookala-argobots/argobots"
else
    ABT_DIR="/home/poornimans/argobots-src/argobots"
fi

function compile_abt() {
    cd "${ABT_DIR}"
    make
    make install
}

echo "Compiling Argobots..."
compile_abt
echo "Compiling benchmarks..."

cd "/home/poornimans/pnookala-argobots/argobots/examples"
clang -g -O3 barrier_test.c -lm -I/home/poornimans/argobots-install/include -L/home/poornimans/argobots-install/lib -labt -o barrier_test
clang -g -O3 noop.c -lm -I/home/poornimans/argobots-install/include -L/home/poornimans/argobots-install/lib -labt -o noop
clang -g -O3 strassen_thread.c -lm -I/home/poornimans/argobots-install/include -L/home/poornimans/argobots-install/lib -labt -o strassen_thread
clang -g -O3 nested_abt.c -lm -I/home/poornimans/argobots-install/include -L/home/poornimans/argobots-install/lib -labt -o nested_abt

bash scripts/noop_basic_test.sh ${test_type}
