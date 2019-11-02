#rm -f strassen_thread
#rm -f strassen_task
#rm -f simple_omptask
#rm -f abt_with_abt_test
clang -g -O3 nested_abt.c -lm -I/home/poornimans/argobots-install/include -L/home/poornimans/argobots-install/lib -labt -o nested_abt
clang -g -O3 barrier_test.c -lm -I/home/poornimans/argobots-install/include -L/home/poornimans/argobots-install/lib -labt -o barrier_test
clang -g -O3 noop.c -lm -I/home/poornimans/argobots-install/include -L/home/poornimans/argobots-install/lib -labt -o noop
clang -g -O3 matrixmul.c -lm -I/home/poornimans/argobots-install/include -L/home/poornimans/argobots-install/lib -labt -o matrixmul
#clang -g abt_with_abt_test.c -I/home/poornimans/argobots-install/include -L/home/poornimans/argobots-install/lib -labt -o abt_with_abt_test
#clang -g simple_omptask.c -I/home/poornimans/argobots-install/include -I/home/poornimans/bolt-install/include -L/home/poornimans/argobots-install/lib -L/home/poornimans/bolt-install/lib -fopenmp -labt -o simple_omptask
clang -g -O3 -march=native  strassen_omp_task.c -lm -I/home/poornimans/argobots-install/include -I/home/poornimans/bolt-install/include -L/home/poornimans/argobots-install/lib -L/home/poornimans/bolt-install/lib -fopenmp -labt -o strassen_task
clang -g -O3 -march=native  strassen_thread.c -lm -I/home/poornimans/argobots-install/include -L/home/poornimans/argobots-install/lib -labt -o strassen_thread
