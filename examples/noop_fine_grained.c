#include <stdio.h>
#include <stdlib.h>
#include <abt.h>
#include <unistd.h>
#include <time.h>
#define NUM_ES          1
#define NUM_ITERATIONS  10000 //100000000
#define NUM_REPEAT      1

typedef unsigned long long ticks;

static inline ticks getticks(void) {
    ticks tsc;
    asm volatile(
            "rdtscp;"
            "shl $32, %%rdx;"
            "or %%rdx, %%rax"
            : "=a"(tsc)
            :
            : "%rcx", "%rdx");

    return tsc;
}

void noop(void* arg) {
    for(int i=0; i<NUM_ITERATIONS; i++) {
        asm volatile("");
    }
}

int main(int argc, char** argv) {
  int i;
  int num_threads, loop_count, ret;
    char* summary_file = NULL;
#ifdef SHARED
  ABT_pool pool;
#else
  ABT_pool *pools;
#endif
    if(argc == 4) {
        num_threads = atoi(argv[1]);
        loop_count = atoi(argv[2]);
        summary_file = argv[3];
    }
    else if(argc == 3)  {
        num_threads = atoi(argv[1]);
        loop_count = atoi(argv[2]);
    }
    else {
        printf("USAGE: ./noop <#ESs> <#ULTs> <Summary file>\n");
        exit(-1);
    }

  
  struct timespec tstart, tend;
  ABT_xstream *xstreams = NULL;
  ABT_thread *threads = NULL;
  int set_main_sched_err;
  int start_i = 0;
  double *diff_time = (double*)malloc(sizeof(double) * NUM_REPEAT);
  ticks start_ticks, end_ticks;

  clock_gettime(CLOCK_MONOTONIC, &tstart);
  noop(NULL);
  clock_gettime(CLOCK_MONOTONIC, &tend);
  
  double noop_time =(tend.tv_sec - tstart.tv_sec) + 
                    ((tend.tv_nsec - tstart.tv_nsec) / 1E9);
  /* Repeat to get stable results */
  ABT_init(0, NULL);
  for (int count = 0; count < NUM_REPEAT; count++) {
    clock_gettime(CLOCK_MONOTONIC, &tstart);
    //ABT_init(0, NULL);
    pools = (ABT_pool *)malloc(sizeof(ABT_pool) * num_threads);
    /* ES creation */
    xstreams = (ABT_xstream *)malloc(sizeof(ABT_xstream) * num_threads);
 
    ABT_xstream_self(&xstreams[0]);
    for (i = 1; i < num_threads; i++) {
        ABT_xstream_create(ABT_SCHED_NULL, &xstreams[i]);
    } 

    for (i = 0; i < num_threads; i++) {
        ABT_xstream_get_main_pools(xstreams[i], 1, &pools[i]);
    }
   
    /* ULT creation */
    threads = (ABT_thread *)malloc(sizeof(ABT_thread) * loop_count);
    int each = loop_count/num_threads;

    for (i = 0; i < num_threads; i++) {
        for(int j = 0; j < each; j++) {
            ret = ABT_thread_create(pools[i], noop, NULL,
                                        ABT_THREAD_ATTR_NULL, &threads[i*each+j]);
        }
    }

    /* join ULTs */
    for (i = 0; i < loop_count; i++) {
        ABT_thread_free(&threads[i]);
    }
    
    /* join ESs */
    for (i = 1; i < num_threads; i++)
    {
	    ABT_xstream_join(xstreams[i]);
        ABT_xstream_free(&xstreams[i]);
    }
    
    clock_gettime(CLOCK_MONOTONIC, &tend);
    diff_time[count] = (tend.tv_sec - tstart.tv_sec) +
                        ((tend.tv_nsec - tstart.tv_nsec) / 1E9);
  }
  ABT_finalize();
  free(pools);
  free(threads);
  free(xstreams);
  
  if(summary_file != NULL) {
    FILE *afp = fopen(summary_file, "a");
    for(int count = 0; count < NUM_REPEAT; count++) {
        printf("%d %d %lf %lf\n", num_threads, loop_count, noop_time, diff_time[count]);
        fprintf(afp, "%d %d %lf %lf\n", num_threads, loop_count, noop_time, diff_time[count]);
    }    
    fclose(afp);
  }
}
