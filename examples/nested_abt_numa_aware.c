#include <stdio.h>
#include <stdlib.h>
#include <abt.h>
#include <unistd.h>
#include <math.h>
#include <sys/time.h>
#include <immintrin.h>

#define SIZE 5184

typedef double real_t;
static int main_num_es = 4;
static int inner_num_es = 4;

real_t * matA; 
real_t * matB; 
real_t * matC;
real_t * result;

int numa0[] = {0,1,2,3,4,5,6,7,8,36,37,38,39,40,41,42,43,44};
int numa1[] = {9,10,11,12,13,14,15,16,17,45,46,47,48,49,50,51,52,53};
int numa2[] = {18,19,20,21,22,23,24,25,26,54,55,56,57,58,59,60,61,62};
int numa3[] = {27,28,29,30,31,32,33,34,35,63,64,65,66,67,68,69,70,71};

int toplevelnum = 0;

typedef struct {
	int start_i;
	int end_i;
    real_t *matC;
} threadData;

typedef void (*inner_f)(void*);

void work_f(void* data) {
	//for(int i = 0; i < 100000000; i++) {
        asm volatile("");
    //}
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
  threadData* arg;
} abt_thread_data_t;

void inner_func_wrapper(void *arg) {
  abt_thread_data_t *d = (abt_thread_data_t *)arg;
  d->inner_func(d->arg);
}

void abt_for(int num_threads, int loop_count, inner_f inner_func, int level, 
                            int* numa, int numasize) {
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
  int numa_idx = 0;
  if(level == 0) {
    //printf("called xstream_self\n");
    ABT_xstream_self(&xstreams[0]);
  
    //set_main_sched_err = ABT_xstream_set_main_sched_basic(xstreams[0],
    //                                                      ABT_SCHED_DEFAULT, 1,
    //                                                      &pool);
    //start_i = (set_main_sched_err != ABT_SUCCESS) ? 0 : 1;
    start_i = 1;
    numa_idx = 1;
  }
    for (i = start_i; i < num_threads; i++) {
    //ABT_xstream_create_basic(ABT_SCHED_DEFAULT, 1, &pool, ABT_SCHED_CONFIG_NULL,
    //                         &xstreams[i]);
    //ABT_xstream_start(xstreams[i]);
    if(level == 0)
        ABT_xstream_create(ABT_SCHED_NULL, &xstreams[i]);
    else {
        ABT_xstream_create_with_rank(ABT_SCHED_NULL, numa[numa_idx], &xstreams[i]);
        numa_idx++;
        if(numa_idx == numasize) numa_idx = 0;
    }
  }

  for (i = 0; i < num_threads; i++) {
    ABT_xstream_get_main_pools(xstreams[i], 1, &pools[i]);
  }

  /* ULT creation */
  threads = (abt_thread_data_t *)malloc(sizeof(abt_thread_data_t) * loop_count);
  int each, leftover;
  
  if(level == 0) {
    each = SIZE;
    leftover = 0;
  }
  else {
    each = SIZE/loop_count;
    leftover = SIZE % loop_count;
  }
  if(level == 0) {
    //printf("Each thread processes %d size matrix\n", each);
  
    for (i = 0; i < loop_count; i++) {
        threads[i].inner_func = inner_func;
        threads[i].arg = (threadData*)malloc(sizeof(threadData));
        threads[i].arg->start_i = i * each;
        threads[i].arg->end_i = threads[i].arg->start_i + each;
        if( i == loop_count - 1)
            threads[i].arg->end_i = threads[i].arg->end_i + leftover;
        //printf("start_i %d, end_i %d\n", threads[i].arg->start_i, threads[i].arg->end_i);
        ABT_thread_create(pools[i], inner_func_wrapper, &threads[i],
                      ABT_THREAD_ATTR_NULL, &threads[i].thread);
    }
  }
  else {
   
    for (i = 0; i < loop_count; i++) {
        real_t *matC = (real_t*)malloc(sizeof(real_t) * SIZE * SIZE);
        threads[i].inner_func = inner_func;
        threads[i].arg = (threadData*)malloc(sizeof(threadData));
        threads[i].arg->start_i = i * each;
        threads[i].arg->end_i = threads[i].arg->start_i + each;
        if( i == loop_count - 1)
            threads[i].arg->end_i = threads[i].arg->end_i + leftover;
        threads[i].arg->matC = matC;
        //printf("start_i %d, end_i %d\n", threads[i].arg->start_i, threads[i].arg->end_i);
        ABT_thread_create(pools[i], inner_func_wrapper, &threads[i],
                      ABT_THREAD_ATTR_NULL, &threads[i].thread);
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
  abt_for(inner_num_es, inner_num_es, work_f, 1, NULL, 0);
}

void inner_par(void* data) {
  //printf("inner abt_for\n");
  int num = __sync_add_and_fetch(&toplevelnum, 1);
    switch(num) {
    case 1:
        abt_for(inner_num_es, inner_num_es, work_f, 1, numa0, 18);
        break;
    case 2:
        abt_for(inner_num_es, inner_num_es, work_f, 1, numa1, 18);
        break;
    case 3:
        abt_for(inner_num_es, inner_num_es, work_f, 1, numa2, 18);
        break;
    case 4: 
        abt_for(inner_num_es, inner_num_es, work_f, 1, numa3, 18);
        break;
    default: 
        abt_for(inner_num_es, inner_num_es, work_f, 1, NULL, 0);
        break;
    }
  //abt_for(inner_num_es, inner_num_es, work_f, 1);
}

void matmul_serial()
{
    for (int64_t j = 0; j < SIZE; j++)
        for (int64_t i = 0; i < SIZE; i++)
            for (int64_t k = 0; k < SIZE; k++)
                result[i + j * SIZE] += matA[k + j * SIZE] * matB[i + k * SIZE];
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


    matA = (real_t *)malloc(sizeof(real_t) * SIZE * SIZE);
    matB = (real_t *)malloc(sizeof(real_t) * SIZE * SIZE);
    matC = (real_t *)malloc(sizeof(real_t) * SIZE * SIZE);
    result = (real_t *)malloc(sizeof(real_t) * SIZE * SIZE);
    for (int64_t j = 0; j < SIZE; j++) {
        for (int64_t i = 0; i < SIZE; i++) {
            // Any random values
            matA[i + j* SIZE] = sin((double)(i + j));
            matB[i + j* SIZE] = cos((double)(i + 2 * j));
            matC[i + j* SIZE] = 0.0;
            result[i + j* SIZE] = 0.0;
        }
    }

  struct timeval start;
  struct timeval stop;
  gettimeofday(&start, NULL);
  abt_for(main_num_es, main_num_es, inner_par, 0, NULL, 0);
  gettimeofday(&stop, NULL);

  /* Verify */
  /*8matmul_serial();
 
  for (int64_t j = 0; j < SIZE; j++) {
            for (int64_t i = 0; i < SIZE; i++) {
                if ((matC[i + j * SIZE] - result[i + j * SIZE]) > (real_t)0.0001) {
                    printf("Different: c[%d,%d] (=%f) != %f\n", (int)i, (int)j,
                           matC[i + j * SIZE], result[i + j * SIZE]);
                    break;
                }
            }
        }
  */
  float elapsed_time = (float)(stop.tv_sec - start.tv_sec + 
                (stop.tv_usec - start.tv_usec)/(float)1000000);  
    
  FILE *afp = fopen(summary_file, "a"); 
  printf("%d %d %f\n", main_num_es, inner_num_es, elapsed_time);
  fprintf(afp, "%d %d %f\n", main_num_es, inner_num_es, elapsed_time);
  fclose(afp);
    //printf("Finished!\n"); 
  return 0;
    
}
