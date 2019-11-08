#include <stdio.h>
#include <stdlib.h>
#include <abt.h>
#include <unistd.h>
#include <sys/time.h>
#define NUM_ES  1
#define NUM_ITERATIONS 100000000

typedef unsigned long long ticks;

static inline ticks getticks(void) {
    ticks tsc;
    asm volatile(
            "rdtsc;"
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
#ifdef SHARED
  ABT_pool pool;
#else
  ABT_pool *pools;
#endif
  int num_threads, loop_count, ret;
    char* summary_file = NULL;

    if(argc == 3) {
        num_threads = atoi(argv[1]);
        summary_file = argv[2];
    }
    else {
        printf("USAGE: <#ESs> <Summary File>\n");
        exit(-1);
    }
  
  struct timeval start;
  struct timeval stop;
  ABT_xstream *xstreams = NULL;
  /*ticks* create_start;
  ticks*  create_end;
  ticks* join_start;
  ticks*  join_end;
  ticks* free_start;
  ticks*  free_end;*/
  ticks start_ticks = 0;
  ticks end_ticks = 0;
  ABT_thread *threads = NULL;
  int set_main_sched_err;
  int start_i = 0;
  int initialized = ABT_initialized() != ABT_ERR_UNINITIALIZED;
  /* initialization */
  gettimeofday(&start, NULL);
  ABT_init(0, NULL);

  
  /* shared pool creation */
#ifdef SHARED
  ABT_pool_create_basic(ABT_POOL_FIFO, ABT_POOL_ACCESS_MPMC, ABT_TRUE, &pool);
#else
  pools = (ABT_pool *)malloc(sizeof(ABT_pool) * num_threads);
#endif  
  /* ES creation */
  xstreams = (ABT_xstream *)malloc(sizeof(ABT_xstream) * num_threads);
  /*create_start = (ticks*)malloc(sizeof(ticks)*num_threads);
  join_start = (ticks*)malloc(sizeof(ticks)*num_threads);
  free_start = (ticks*)malloc(sizeof(ticks)*num_threads);
  
  create_end = (ticks*)malloc(sizeof(ticks)*num_threads);
  join_end = (ticks*)malloc(sizeof(ticks)*num_threads);
  free_end = (ticks*)malloc(sizeof(ticks)*num_threads);
  */
  ABT_xstream_self(&xstreams[0]);
#ifdef SHARED
    set_main_sched_err = ABT_xstream_set_main_sched_basic(xstreams[0],
                                                          ABT_SCHED_DEFAULT, 1,
                                                          &pool);
    start_i = (set_main_sched_err != ABT_SUCCESS) ? 0 : 1;
#endif
  start_ticks = getticks();
  for (i = 1; i < num_threads; i++) {
#ifdef SHARED
    ABT_xstream_create_basic(ABT_SCHED_DEFAULT, 1, &pool, ABT_SCHED_CONFIG_NULL,
                             &xstreams[i]);
    ABT_xstream_start(xstreams[i]);
#else
    ABT_xstream_create(ABT_SCHED_NULL, &xstreams[i]);
    ABT_xstream_join(xstreams[i]);
    ABT_xstream_free(&xstreams[i]);
#endif
  }
  end_ticks = getticks();
/*#ifndef SHARED
  for (i = 0; i < num_threads; i++) {
    ABT_xstream_get_main_pools(xstreams[i], 1, &pools[i]);
  }
#endif*/
  /* ULT creation */
  /*threads = (ABT_thread *)malloc(sizeof(ABT_thread) * loop_count);
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
*/
//  printf("threads created...joining threads!\n");
  /* join ULTs */
  /*for (i = 0; i < loop_count; i++) {
    //printf("joining thread %d\n", i);
    ABT_thread_free(&threads[i]);
  }*/

  /* join ESs */
#if 0
  // if the following starts from 1, ABT_finalize will cause an error since the
  // main ULT might be scheduled by a secondary execuntion stream.
  for (i = 1; i < num_threads; i++)
#else
    //printf("calling join on xstreams\n");
    //for (i = 1; i < num_threads; i++)
#endif
     /* {
        //join_start[i-1] = getticks();
	    ABT_xstream_join(xstreams[i]);
        //join_end[i-1] = getticks();
        //free_start[i-1] = join_end[i-1];
	    ABT_xstream_free(&xstreams[i]);
        //free_end[i-1] = getticks();
    }*/

  //end_ticks = getticks();

  ABT_finalize();
  gettimeofday(&stop, NULL);
  float elapsed_time = (float)(stop.tv_sec - start.tv_sec +
                (stop.tv_usec - start.tv_usec)/(float)1000000);
   
  /*double create = 0.0;
  double join = 0.0;
  double free = 0.0;
  for(int i = 0; i < num_threads-1; i++) {
    create += create_end[i] - create_start[i];
    join += join_end[i] - join_start[i];
    free += free_end[i] - free_start[i];
  }*/ 

  if(summary_file != NULL) {
        FILE *afp = fopen(summary_file, "a");
        /*printf("%d %f %f %f\n", num_threads, (create)/(num_threads-1),
                            (join)/(num_threads-1),
                            (free)/(num_threads-1));
        fprintf(afp, "%d %f %f %f\n", num_threads, (create)/(num_threads-1),
                            (join)/(num_threads-1),
                            (free)/(num_threads-1));*/
        printf("%d %f\n", num_threads, (end_ticks-start_ticks) * 1.0/(num_threads-1));
        fprintf(afp, "%d %f\n", num_threads, (end_ticks-start_ticks) * 1.0/(num_threads-1));
        fclose(afp);
  }
}

