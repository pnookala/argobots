#include <stdio.h>
#include <stdlib.h>
#include <abt.h>
#include <unistd.h>
#include <math.h>
#include <sys/time.h>
#include <immintrin.h>
#include <time.h>

#define NUM_REPEATS 1

typedef double real_t;
static int main_num_es = 4;
static int inner_num_es = 4;
static long int num_iterations = 1;
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

typedef void (*inner_f)(void*);

void work_f(void* data) {
    for(int i = 0; i < num_iterations; i++) {
        asm volatile("");
    }
/*    threadData* args = (threadData*)data;
    for (int64_t j = args->start_i; j < args->end_i; j++)
    {
        for (int64_t i = 0; i < SIZE; i++)
        {
            //__m128d c = _mm_setzero_pd();
            for (int64_t k = 0; k < SIZE; k += 1)
                //c = _mm_add_pd(c, _mm_mul_pd(
                //        _mm_load_pd(&matA[k + j * SIZE]),
                //        _mm_load_pd(&matB[i + k * SIZE])));
                args->matC[i + j * SIZE] += matA[k + j * SIZE] * matB[i + k * SIZE];
            //c = _mm_add_pd(c, c);
            //_mm_store_sd(&matC[i + j * SIZE], c);
        }
    }
*/
    /*for (int i = data->start_i; i < data->end_i; i++) { 
         for (int j = 0; j < SIZE; j++)  
                for (int k = 0; k < SIZE; k++)  
    	          matC[i][j] += matA[i][k] * matB[k][j];
         
    }*/

}

typedef struct {
  ABT_thread thread;
  inner_f inner_func;
} abt_thread_data_t;

void inner_func_wrapper(void *arg) {
  abt_thread_data_t *d = (abt_thread_data_t *)arg;
  d->inner_func(NULL);
}

void ults_for(int num_threads, int loop_count, inner_f inner_func) {
  abt_thread_data_t *threads = NULL;
  threads = (abt_thread_data_t *)malloc(sizeof(abt_thread_data_t) * loop_count);
  int i;
  ABT_thread self;
  ABT_thread_self(&self);
  ABT_pool p = ABT_POOL_NULL;
  
  ABT_thread_get_last_pool(self, &p);
  int each = loop_count/num_threads;
  for (i = 0; i < num_threads; i++) {
      for(int j = 0; j < each; j++) {
        threads[i*each+j].inner_func = inner_func;
        ABT_thread_create(p, inner_func_wrapper, &threads[i*each+j],
                      ABT_THREAD_ATTR_NULL, &threads[i*each+j].thread);
      }
      
    }

  /* join ULTs */
  for (i = 0; i < loop_count; i++) {
    ABT_thread_free(&threads[i].thread);
  }
} 

void abt_for(int num_threads, int loop_count, inner_f inner_func, int level) {
  int i;
  //ABT_pool pool;
  ABT_pool *pools;
  ABT_xstream *xstreams = NULL;
  abt_thread_data_t *threads = NULL;
  int set_main_sched_err;
  int start_i = 0;
  int initialized = ABT_initialized() != ABT_ERR_UNINITIALIZED;
  /* initialization */
  ABT_init(0, NULL);

  pools = (ABT_pool *)malloc(sizeof(ABT_pool) * num_threads);
  /* ES creation */
  xstreams = (ABT_xstream *)malloc(sizeof(ABT_xstream) * num_threads);

  if(level == 0) {
    ABT_xstream_self(&xstreams[0]);
    start_i = 1;
  }
  for (i = start_i; i < num_threads; i++) {
        ABT_xstream_create(ABT_SCHED_NULL, &xstreams[i]);
    }

  for (i = 0; i < num_threads; i++) {
    ABT_xstream_get_main_pools(xstreams[i], 1, &pools[i]);
  }

  if(level == 0) {
    ABT_xstream_set_main_sched_basic(xstreams[0],
                                                          ABT_SCHED_DEFAULT, 1,
                                                          &pools[0]);
  }

  /* ULT creation */
  threads = (abt_thread_data_t *)malloc(sizeof(abt_thread_data_t) * loop_count);
    int each = loop_count/num_threads; 
    for (i = 0; i < loop_count; i++) {
      for(int j = 0; j < each; j++) {
        threads[i*each+j].inner_func = inner_func;
        ABT_thread_create(pools[i], inner_func_wrapper, &threads[i*each+j],
                      ABT_THREAD_ATTR_NULL, &threads[i*each+j].thread);
      }
    }

  /* join ULTs */
  for (i = 0; i < loop_count; i++) {
    ABT_thread_free(&threads[i].thread);
  }
  /* join ESs */
  for (i = start_i; i < num_threads; i++)
    {
	    ABT_xstream_join(xstreams[i]);
    }
  ABT_finalize();
}

void inner2_par(int i) {
  abt_for(inner_num_es, inner_num_es, work_f, 1);
}

void inner_par(void* data) {
#ifdef USE_NESTED_ULTS
  ults_for(inner_num_es, inner_num_es,  work_f);
#else
  abt_for(inner_num_es, inner_num_es, work_f, 1);
#endif
}

int main (int argc, char** argv) {
  char* summary_file = NULL;
  if(argc != 5) {
    printf("Usage: <#Main ESs> <#Inner ESs> <#NOPS> <Summary File>\n");
    exit(-1);
  } 
  else {
    main_num_es = atoi(argv[1]);
    inner_num_es = atoi(argv[2]);
    num_iterations = atoi(argv[3]);
    summary_file = argv[4];
  }

  /* Calibrate NOOPs */
  ticks noop_start = getticks();
  work_f(NULL);
  ticks noop_ticks = getticks() - noop_start;
  struct timespec tstart;
  struct timespec tend;
  double *elapsed_time = (double*)malloc(sizeof(double) * NUM_REPEATS);
 
  //ticks start_ticks = getticks();
  for (int count = 0; count < NUM_REPEATS; count++) {
    clock_gettime(CLOCK_MONOTONIC, &tstart);
    abt_for(main_num_es, main_num_es, inner_par, 0);
    clock_gettime(CLOCK_MONOTONIC, &tend);

   elapsed_time[count] =(tend.tv_sec - tstart.tv_sec)*1000 +
                    ((tend.tv_nsec - tstart.tv_nsec) / 1E6);
  }
  //ticks end_ticks = getticks();
  FILE *afp = fopen(summary_file, "a"); 
  for(int count=0; count < NUM_REPEATS; count++) {
  printf("%d %d %ld %llu %f\n", main_num_es, inner_num_es, num_iterations, 
                    noop_ticks, elapsed_time[count]);
  fprintf(afp, "%d %d %ld %llu %f\n", main_num_es, inner_num_es, num_iterations,
                    noop_ticks, elapsed_time[count]);
  }
  fclose(afp);
  return 0;
    
}
