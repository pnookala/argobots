#include <stdio.h>
#include <stdlib.h>
#include <abt.h>
#include <unistd.h>
#include <sys/time.h>

static int main_num_es = 4;
static int inner_num_es = 4;

typedef struct {
	int start_i;
	int end_i;
} threadData;

typedef void (*inner_f)(void*);

void work_f(void* args) {
	//printf("Hello World\n");
	for(int i = 0; i < 100000000; i++) {
        asm volatile("");
    }
    /*threadData* data = (threadData*)args;
  
    for (int i = data->start_i; i < data->end_i; i++)  
         for (int j = 0; j < SIZE; j++)  
                for (int k = 0; k < SIZE; k++)  
    	          matC[i][j] += matA[i][k] * matB[k][j];
*/
}

typedef struct {
  ABT_thread thread;
  inner_f inner_func;
  threadData* arg;
} abt_thread_data_t;

void inner_func_wrapper(void *arg) {
  abt_thread_data_t *d = (abt_thread_data_t *)arg;
  d->inner_func(d->arg);
}

void abt_for(int num_threads, int loop_count, inner_f inner_func, int level) {
  int i;
  ABT_pool *pools;
  //ABT_pool pool;
  ABT_xstream *xstreams = NULL;
  abt_thread_data_t *threads = NULL;
  int set_main_sched_err;
  int start_i = 0;
  int initialized = ABT_initialized() != ABT_ERR_UNINITIALIZED;
  /* initialization */
  ABT_init(0, NULL);

  /* shared pool creation */
  //ABT_pool_create_basic(ABT_POOL_FIFO, ABT_POOL_ACCESS_MPMC, ABT_TRUE, &pool);

  pools = (ABT_pool *)malloc(sizeof(ABT_pool) * num_threads);
  /* ES creation */
  xstreams = (ABT_xstream *)malloc(sizeof(ABT_xstream) * num_threads);
  if(level == 0) {
    //printf("called xstream_self\n");
    ABT_xstream_self(&xstreams[0]);
  
    //set_main_sched_err = ABT_xstream_set_main_sched_basic(xstreams[0],
    //                                                      ABT_SCHED_DEFAULT, 1,
    //                                                      &pool);
    //start_i = (set_main_sched_err != ABT_SUCCESS) ? 0 : 1;
    start_i = 1;
  }
    for (i = start_i; i < num_threads; i++) {
    //ABT_xstream_create_basic(ABT_SCHED_DEFAULT, 1, &pool, ABT_SCHED_CONFIG_NULL,
    //                         &xstreams[i]);
    //ABT_xstream_start(xstreams[i]);
    ABT_xstream_create(ABT_SCHED_NULL, &xstreams[i]);
  }

  for (i = 0; i < num_threads; i++) {
    ABT_xstream_get_main_pools(xstreams[i], 1, &pools[i]);
  }

  /* ULT creation */
  threads = (abt_thread_data_t *)malloc(sizeof(abt_thread_data_t) * loop_count);
  int num_ult_per_pool = loop_count/num_threads;
  //printf("Each thread processes %d size matrix\n", each);
  for (i = 0; i < num_threads; i++) {
     for(int j = 0; j < num_ult_per_pool; j++) {
  //      threads[i].inner_func = inner_func;
  //      threads[i].arg = (threadData*)malloc(sizeof(threadData));
  //      threads[i].arg->start_i = i * each;
  //      threads[i].arg->end_i = threads[i].arg->start_i + each - 1;
        //printf("start_i %d, end_i %d\n", threads[i].arg->start_i, threads[i].arg->end_i);
        ABT_thread_create(pools[i], inner_func, NULL,
                      ABT_THREAD_ATTR_NULL, &threads[i*num_ult_per_pool+j].thread);
    }
  }

//  printf("threads created...joining threads!\n");
  /* join ULTs */
  for (i = 0; i < loop_count; i++) {
    //printf("joining thread %d\n", i);
    ABT_thread_free(&threads[i].thread);
  }

  /* join ESs */
#if 0
  // if the following starts from 1, ABT_finalize will cause an error since the
  // main ULT might be scheduled by a secondary execuntion stream.
  for (i = 1; i < num_threads; i++)
#else
    //printf("calling join on xstreams\n");
    for (i = start_i; i < num_threads; i++)
#endif
      {
        //printf("join %d\n", i); 
	    ABT_xstream_join(xstreams[i]);
	    //ABT_xstream_free(&xstreams[i]);
    }

  //printf("call finalize\n");
  ABT_finalize();
}

void inner2_par(int i) {
  abt_for(inner_num_es, inner_num_es, work_f, 1);
}

void inner_par(void* data) {
  printf("inner abt_for\n");
  abt_for(inner_num_es, inner_num_es*144, work_f, 1);
}

int main (int argc, char** argv) {
  char* summary_file = NULL;
  if(argc != 4) {
    printf("Usage: <#Main ESs> <#Inner ESs> <Summary File>\n");
    exit(-1);
  } 
  else {
    main_num_es = atoi(argv[1]);
    inner_num_es = atoi(argv[2]);
    summary_file = argv[3];
  }
  struct timeval start;
  struct timeval stop;
  gettimeofday(&start, NULL);
  abt_for(main_num_es, main_num_es, inner_par, 0);
  gettimeofday(&stop, NULL);

  float elapsed_time = (float)(stop.tv_sec - start.tv_sec + 
                (stop.tv_usec - start.tv_usec)/(float)1000000);  
    
  FILE *afp = fopen(summary_file, "a"); 
  printf("%d %d %f\n", main_num_es, inner_num_es, elapsed_time);
  fprintf(afp, "%d %d %f\n", main_num_es, inner_num_es, elapsed_time);
  fclose(afp);
    //printf("Finished!\n"); 
  return 0;
    
}
