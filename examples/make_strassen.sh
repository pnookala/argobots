#rm -f strassen_thread
#rm -f strassen_task
#rm -f simple_omptask
#rm -f abt_with_abt_test
#clang -O3 deadlock_test.c -lm -I/home/poornimans/argobots-install/include -L/home/poornimans/argobots-install/lib -labt -o deadlock_test
clang -O3 -DUSE_NESTED_ULTS  nested_abt.c -march=native -lm -I/home/poornimans/argobots-install/include -L/home/poornimans/argobots-install/lib -labt -o nested_abt
#clang -O3 barrier_test.c -lm -I/home/poornimans/argobots-install/include -L/home/poornimans/argobots-install/lib -labt -o barrier_test
clang -g -O3 noop.c -lm -I/home/poornimans/argobots-install/include -L/home/poornimans/argobots-install/lib -labt -o noop
#clang -O3 matrixmul.c -lm -I/home/poornimans/argobots-install/include -L/home/poornimans/argobots-install/lib -labt -o matrixmul
clang -O3 ves_create.c -lm -I/home/poornimans/argobots-install/include -L/home/poornimans/argobots-install/lib -labt -o ves_create
#clang -g abt_with_abt_test.c -I/home/poornimans/argobots-install/include -L/home/poornimans/argobots-install/lib -labt -o abt_with_abt_test
#clang -g simple_omptask.c -I/home/poornimans/argobots-install/include -I/home/poornimans/bolt-install/include -L/home/poornimans/argobots-install/lib -L/home/poornimans/bolt-install/lib -fopenmp -labt -o simple_omptask
#clang -O3 -march=native  strassen_omp_task.c -lm -I/home/poornimans/argobots-install/include -I/home/poornimans/bolt-install/include -L/home/poornimans/argobots-install/lib -L/home/poornimans/bolt-install/lib -fopenmp -labt -o strassen_task
clang -O3 ult_fork_join.c -lm -I/home/poornimans/argobots-install/include -L/home/poornimans/argobots-install/lib -labt -o ult_fork_join
#clang -O3 -march=native  strassen_thread.c -lm -I/home/poornimans/argobots-install/include -L/home/poornimans/argobots-install/lib -labt -o strassen_thread
