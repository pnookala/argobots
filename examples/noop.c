#include <stdio.h>
#include <stdlib.h>
#include <abt.h>
#include <unistd.h>
#include <time.h>
#define NUM_ES          1
#define NUM_ITERATIONS  1 //100000000
#define NUM_REPEAT      100

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
        num_threads = NUM_ES;
        loop_count = NUM_ES;
    }
  
  struct timespec tstart, tend;
  ABT_xstream *xstreams = NULL;
  ABT_thread *threads = NULL;
  int set_main_sched_err;
  int start_i = 0;
  double *diff_time = (double*)malloc(sizeof(double) * NUM_REPEAT);
  /* initialization */
  clock_gettime(CLOCK_MONOTONIC, &tstart);
  noop(NULL);
  clock_gettime(CLOCK_MONOTONIC, &tend);
  
  double noop_time =(tend.tv_sec - tstart.tv_sec)*1000 + 
                    ((tend.tv_nsec - tstart.tv_nsec) / 1E6);
  
  /* Repeat to get stable results */
  for (int count = 0; count < NUM_REPEAT; count++) {
    clock_gettime(CLOCK_MONOTONIC, &tstart);
    ABT_init(0, NULL);
  /* shared pool creation */
#ifdef SHARED
  ABT_pool_create_basic(ABT_POOL_FIFO, ABT_POOL_ACCESS_MPMC, ABT_TRUE, &pool);
#else
  pools = (ABT_pool *)malloc(sizeof(ABT_pool) * num_threads);
#endif  
  /* ES creation */
  xstreams = (ABT_xstream *)malloc(sizeof(ABT_xstream) * num_threads);
  
  ABT_xstream_self(&xstreams[0]);
#ifdef SHARED
    set_main_sched_err = ABT_xstream_set_main_sched_basic(xstreams[0],
                                                          ABT_SCHED_DEFAULT, 1,
                                                          &pool);
    start_i = (set_main_sched_err != ABT_SUCCESS) ? 0 : 1;
#endif
  
  for (i = 1; i < num_threads; i++) {
#ifdef SHARED
    ABT_xstream_create_basic(ABT_SCHED_DEFAULT, 1, &pool, ABT_SCHED_CONFIG_NULL,
                             &xstreams[i]);
    ABT_xstream_start(xstreams[i]);
#else
    ABT_xstream_create(ABT_SCHED_NULL, &xstreams[i]);
#endif
  }

#ifndef SHARED
  for (i = 0; i < num_threads; i++) {
    ABT_xstream_get_main_pools(xstreams[i], 1, &pools[i]);
  }
#endif
  /* ULT creation */
  threads = (ABT_thread *)malloc(sizeof(ABT_thread) * loop_count);
  int each = loop_count/num_threads;
    for (i = 0; i < num_threads; i++) {
    for(int j = 0; j < each; j++) {
#ifdef SHARED
        ret = ABT_thread_create(pool, noop, NULL, ABT_THREAD_ATTR_NULL,
                                        &threads[i*each+j]);
#else
        ret = ABT_thread_create(pools[i], noop, NULL,
                                        ABT_THREAD_ATTR_NULL, &threads[i*each+j]);
#endif
    }
  }

//  printf("threads created...joining threads!\n");
  /* join ULTs */
  for (i = 0; i < loop_count; i++) {
    //printf("joining thread %d\n", i);
    ABT_thread_free(&threads[i]);
  }

  /* join ESs */
#if 0
  // if the following starts from 1, ABT_finalize will cause an error since the
  // main ULT might be scheduled by a secondary execuntion stream.
  for (i = 1; i < num_threads; i++)
#else
    //printf("calling join on xstreams\n");
    for (i = 1; i < num_threads; i++)
#endif
      {
	    ABT_xstream_join(xstreams[i]);
	    //ABT_xstream_free(&xstreams[i]);
    }

  //printf("call finalize\n");
   ABT_finalize();
    clock_gettime(CLOCK_MONOTONIC, &tend);
    free(pools);
    free(threads);
    free(xstreams);
    diff_time[count] = (tend.tv_sec - tstart.tv_sec)*1000 + 
                        ((tend.tv_nsec - tstart.tv_nsec) / 1E6);
  }
    //gettimeofday(&stop, NULL);
  //float elapsed_time = (float)(stop.tv_sec - start.tv_sec +
  //              (stop.tv_usec - start.tv_usec)/(float)1000000);

   
  if(summary_file != NULL) {
    FILE *afp = fopen(summary_file, "a");
    for(int count = 0; count < NUM_REPEAT; count++) {
        printf("%d %d %lf %lf\n", num_threads, loop_count, noop_time, diff_time[count]);
        fprintf(afp, "%d %d %lf %lf\n", num_threads, loop_count, noop_time, diff_time[count]);
    }    
    fclose(afp);
  }
}

