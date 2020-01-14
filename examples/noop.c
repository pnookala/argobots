#include <stdio.h>
#include <stdlib.h>
#include <abt.h>
#include <unistd.h>
#include <time.h>
#include "papi.h"

#define NUM_ES          1
#define NUM_ITERATIONS  1 //100 //100000000
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
  ABT_pool *pools;
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

  long long values[3];
  long long elapsed_us, elapsed_cyc;
  double ipc;
  PAPI_library_init(PAPI_VER_CURRENT);

  struct timespec tstart, tend;
  ABT_xstream *xstreams = NULL;
  ABT_thread *threads = NULL;
  int set_main_sched_err;
  int start_i = 0;
  double *diff_time = (double*)malloc(sizeof(double) * NUM_REPEAT);
  ticks start_ticks, end_ticks;

  //clock_gettime(CLOCK_MONOTONIC, &tstart);
  //noop(NULL);
  //clock_gettime(CLOCK_MONOTONIC, &tend);
  
  //double noop_time =(tend.tv_sec - tstart.tv_sec) + 
  //                  ((tend.tv_nsec - tstart.tv_nsec) / 1E9);
  /* Repeat to get stable results */
  int EventSet1 = PAPI_NULL;
  int retval;
  ABT_init(0, NULL);
  for (int count = 0; count < NUM_REPEAT; count++) {
    //clock_gettime(CLOCK_MONOTONIC, &tstart);
    //ABT_init(0, NULL);
    int ret = PAPI_create_eventset(&EventSet1);
    retval=PAPI_add_named_event(EventSet1,"PAPI_TOT_CYC");

    /* Add PAPI_TOT_INS */
    retval=PAPI_add_named_event(EventSet1,"PAPI_TOT_INS");
    /*if (retval!=PAPI_OK) {

        printf( __FILE__, __LINE__, "adding PAPI_TOT_INS", retval );
    }*/
    retval = PAPI_add_named_event(EventSet1, "PAPI_REF_CYC");

    elapsed_us = PAPI_get_real_usec(  );
    elapsed_cyc = PAPI_get_real_cyc(  );

    pools = (ABT_pool *)malloc(sizeof(ABT_pool) * num_threads);
    /* ES creation */
    xstreams = (ABT_xstream *)malloc(sizeof(ABT_xstream) * num_threads);
    
    retval = PAPI_start( EventSet1 );
    //clock_gettime(CLOCK_MONOTONIC, &tstart); 
    
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
    
    //clock_gettime(CLOCK_MONOTONIC, &tend);
    //diff_time[count] = (tend.tv_sec - tstart.tv_sec) +
    //                    ((tend.tv_nsec - tstart.tv_nsec) / 1E9);
    retval = PAPI_stop( EventSet1, values );
    elapsed_us = PAPI_get_real_usec(  ) - elapsed_us;
    elapsed_cyc = PAPI_get_real_cyc(  ) - elapsed_cyc;
    //printf( "%-12s %12lld\n", "PAPI_TOT_CYC : \t", values[0] );
    //printf( "%-12s %12lld\n", "PAPI_TOT_INS : \t", values[1] );
    //printf( "%-12s %12lld\n", "Real usec    : \t", elapsed_us );
    //printf( "%-12s %12lld\n", "Real cycles  : \t", elapsed_cyc );
    /* Shutdown the EventSet */
    retval = PAPI_remove_named_event( EventSet1, "PAPI_TOT_CYC" );

    retval = PAPI_remove_named_event( EventSet1, "PAPI_TOT_INS" );
    retval = PAPI_remove_named_event( EventSet1, "PAPI_REF_CYC" );

    retval=PAPI_destroy_eventset( &EventSet1 );

    /* Calculate Instructions per Cycle, avoiding division by zero */
    if (values[0]!=0) {
        ipc = (double)values[1]/(double)values[0];
    }
    else {
        ipc=0.0;
    }

    //printf( "%-12s %12.2lf\n",  "IPC          : \t", ipc );
  }
  ABT_finalize();
  free(pools);
  free(threads);
  free(xstreams);
  
  if(summary_file != NULL) {
    FILE *afp = fopen(summary_file, "a");
    for(int count = 0; count < NUM_REPEAT; count++) {
        printf( "#ESs : %d\n #ULTs : %d\n Real us : %lld \nReal cycles : %lld \nPAPI_TOT_CYC : %lld \nPAPI_TOT_INS : %lld \n PAPI_REF_CYC : %lld\n IPC : %.2lf\n",
                num_threads, loop_count, elapsed_us, elapsed_cyc, values[0], values[1], values[2],  ipc);
        fprintf(afp, "%d %d %lld %lld %lld %lld %lld %.2lf\n", 
                num_threads, loop_count, elapsed_us, elapsed_cyc, values[0], values[1], values[2], ipc);
    }    
    fclose(afp);
  }
  
    PAPI_shutdown();
    exit(0);
}
