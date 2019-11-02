#include <stdio.h>
#include <stdlib.h>
#include <abt.h>
#include <unistd.h>
#include <sys/time.h>
#define NUM_ES  1
ABT_barrier global_barrier;

typedef struct {
	int id;
} threadData;

void run(void* arg) {
    //threadData* td = (threadData*)arg;
    //printf("Entered %d\n", td->id);
    ABT_barrier_wait(global_barrier);
    //printf("Exiting %d\n", td->id);
}

int main(int argc, char** argv) {
  int i;
  ABT_pool *pools;
  int num_threads, loop_count, ret;
    char* summary_file = NULL;

    if(argc == 4) {
        num_threads = atoi(argv[1]);
        loop_count = atoi(argv[2]);
        summary_file = argv[3];
    }
    else {
        num_threads = atoi(argv[1]);
        loop_count = atoi(argv[2]);
    }
    
  if(loop_count % num_threads != 0) {
    printf("Error: #ULTs must be a multiple of #ESs for this test.\n");
    exit(-1);
  }
  
  struct timeval start;
  struct timeval stop;
  //ABT_pool pool;
  ABT_xstream *xstreams = NULL;
  ABT_thread *threads = NULL;
  int set_main_sched_err;
  int start_i = 0;
  int initialized = ABT_initialized() != ABT_ERR_UNINITIALIZED;
  /* initialization */
  gettimeofday(&start, NULL);
  ABT_init(0, NULL);

  /* shared pool creation */
  //ABT_pool_create_basic(ABT_POOL_FIFO, ABT_POOL_ACCESS_MPMC, ABT_TRUE, &pool);
  ret = ABT_barrier_create((size_t) loop_count, &global_barrier);
  pools = (ABT_pool *)malloc(sizeof(ABT_pool) * num_threads);
  /* ES creation */
  xstreams = (ABT_xstream *)malloc(sizeof(ABT_xstream) * num_threads);
  
  ABT_xstream_self(&xstreams[0]);
  
    //set_main_sched_err = ABT_xstream_set_main_sched_basic(xstreams[0],
    //                                                      ABT_SCHED_DEFAULT, 1,
    //                                                      &pool);
    //start_i = (set_main_sched_err != ABT_SUCCESS) ? 0 : 1;
    for (i = 1; i < num_threads; i++) {
    //ABT_xstream_create_basic(ABT_SCHED_DEFAULT, 1, &pool, ABT_SCHED_CONFIG_NULL,
    //                         &xstreams[i]);
    //ABT_xstream_start(xstreams[i]);
    ABT_xstream_create(ABT_SCHED_NULL, &xstreams[i]);
  }

  for (i = 0; i < num_threads; i++) {
    ABT_xstream_get_main_pools(xstreams[i], 1, &pools[i]);
  }

  /* ULT creation */
  threads = (ABT_thread *)malloc(sizeof(ABT_thread) * loop_count);
  int each = loop_count/num_threads;
  //printf("Each thread processes %d size matrix\n", each);
  for (i = 0; i < num_threads; i++) {
    for(int j = 0; j < each; j++) {
        //threadData* arg = (threadData*)malloc(sizeof(threadData));
        //arg->id = i * each + j;
        //printf("thread id %d\n", arg->id);
    
        ret = ABT_thread_create(pools[i], run, NULL,
                                        ABT_THREAD_ATTR_NULL, &threads[i * each + j]);
  
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
        //printf("join %d\n", i); 
	    ABT_xstream_join(xstreams[i]);
	    //ABT_xstream_free(&xstreams[i]);
    }

  //printf("call finalize\n");
  ABT_finalize();
  gettimeofday(&stop, NULL);
  float elapsed_time = (float)(stop.tv_sec - start.tv_sec +
                (stop.tv_usec - start.tv_usec)/(float)1000000);
    
  if(summary_file != NULL) {
    FILE *afp = fopen(summary_file, "a");
    printf("%d %d %f\n", num_threads, loop_count, elapsed_time);
    fprintf(afp, "%d %d %f\n", num_threads, loop_count, elapsed_time);
    fclose(afp);
  }
}

