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
clang -O3 barrier_test.c -lm -I/home/poornimans/argobots-install/include -L/home/poornimans/argobots-install/lib -labt -o barrier_test
clang -O3 noop.c -lm -I/home/poornimans/argobots-install/include -L/home/poornimans/argobots-install/lib -labt -o noop
clang -O3 strassen_thread.c -lm -I/home/poornimans/argobots-install/include -L/home/poornimans/argobots-install/lib -labt -o strassen_thread
clang  -O3 nested_abt.c -lm -I/home/poornimans/argobots-install/include -L/home/poornimans/argobots-install/lib -labt -o nested_abt
clang -O3 matrixmul.c -lm -I/home/poornimans/argobots-install/include -L/home/poornimans/argobots-install/lib -labt -o matrixmul
clang -O3 ves_create.c -lm -I/home/poornimans/argobots-install/include -L/home/poornimans/argobots-install/lib -labt -o ves_create
#bash scripts/ves_create_test.sh ${test_type}
bash scripts/noop_basic_test.sh ${test_type}
