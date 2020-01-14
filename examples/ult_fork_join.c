#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <abt.h>
#include <unistd.h>
#include <sys/time.h>
#include <sched.h>
#include <papi.h>
#define NUM_ES  1
#define NUM_ITERATIONS 1

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

#define TRACING_SSC_MARK( MARK_ID )                     \
                asm volatile (                                  \
                "\n\t  movl $"#MARK_ID", %%ebx"         \
                "\n\t  .byte 0x64, 0x67, 0x90"          \
                : : : "%ebx","memory" );

void noop(void* arg) {
    //for(int i=0; i<NUM_ITERATIONS; i++) {
        asm volatile("");
    //}
}

int main(int argc, char** argv) {
  int i,k;
  ABT_pool *pools;
  int num_threads, loop_count;
    char* summary_file = NULL;

    if(argc == 4) {
        num_threads = atoi(argv[1]);
        loop_count = atoi(argv[2]);
        summary_file = argv[3];
    }
    else {
        printf("USAGE: <#ESs> <#Iterations> <Summary File>\n");
        exit(-1);
    }
  
  struct timeval start;
  struct timeval stop;
  ABT_xstream *xstreams = NULL;
  ABT_thread *threads = NULL;
  ticks start_ticks;
  ticks end_ticks;
  ticks *diff_ticks;
  float *times;

  long long values[3];
  long long elapsed_us, elapsed_cyc;
  double ipc;
  PAPI_library_init(PAPI_VER_CURRENT);

  times = (float*)calloc(loop_count, sizeof(float));
   /* initialization */
  //  gettimeofday(&start, NULL);
  //cpu_set_t my_cpu;
    /* Skip CPU0 - let the OS run on that one */
    //int my_cpu_num = 10;

    //CPU_ZERO (&my_cpu);
    //CPU_SET (my_cpu_num, &my_cpu);
    //if (sched_setaffinity (0, sizeof(my_cpu), &my_cpu) == -1)
    //    printf ("setaffinity failed\n");
  int EventSet1 = PAPI_NULL;
  int retval;
  int ret = PAPI_create_eventset(&EventSet1);
    retval=PAPI_add_named_event(EventSet1,"PAPI_TOT_CYC");

    /* Add PAPI_TOT_INS */
    retval=PAPI_add_named_event(EventSet1,"PAPI_TOT_INS");
    /*if (retval!=PAPI_OK) {

        printf( __FILE__, __LINE__, "adding PAPI_TOT_INS", retval );
    }*/
    retval = PAPI_add_named_event(EventSet1, "PAPI_REF_CYC");

  ABT_init(0, NULL);
  pools = (ABT_pool *)malloc(sizeof(ABT_pool) * num_threads);
  /* ES creation */
  xstreams = (ABT_xstream *)malloc(sizeof(ABT_xstream) * 72);
    
  //start_ticks = (ticks*)calloc(loop_count, sizeof(ticks));
  //end_ticks = (ticks*)calloc(loop_count, sizeof(ticks));
  //diff_ticks = (ticks*)calloc(NUM_ITERATIONS, sizeof(ticks));

  ABT_xstream_self(&xstreams[0]);
  
  /* warm up */
  for (i = 1; i < num_threads; i++) {
      ABT_xstream_create(ABT_SCHED_NULL, &xstreams[i]);
  }

#ifndef SHARED
  for (i = 0; i < num_threads; i++) {
    ABT_xstream_get_main_pools(xstreams[i], 1, &pools[i]);
  }
#endif

  for(int count = 0; count < NUM_ITERATIONS; count++) {
  /* ULT creation */
  
  threads = (ABT_thread *)malloc(sizeof(ABT_thread) * loop_count);
  int each = loop_count/num_threads;
  /* warm up */
  /*for (i = 0; i < num_threads; i++) {
    ret = ABT_thread_create(pools[i], noop, NULL, ABT_THREAD_ATTR_NULL,
                                        &threads[i]);
  }

  for(i = 0; i < num_threads; i++) {
    ABT_thread_join(&threads[i]);
  }*/
  elapsed_us = PAPI_get_real_usec(  );
    elapsed_cyc = PAPI_get_real_cyc(  );
  retval = PAPI_start( EventSet1 );
  //start_ticks = getticks();
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

  retval = PAPI_stop( EventSet1, values );
    elapsed_us = (PAPI_get_real_usec(  ) - elapsed_us)/(loop_count);
    elapsed_cyc = PAPI_get_real_cyc(  ) - elapsed_cyc;

//  printf("threads created...joining threads!\n");
  /* join ULTs */
  for (i = 0; i < loop_count; i++) {
    //printf("joining thread %d\n", i);
    ABT_thread_join(threads[i]);
  }
  //end_ticks = getticks();
  //diff_ticks[count] = (end_ticks - start_ticks)/loop_count;
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
        //join_start[i-1] = getticks();
	    ABT_xstream_join(xstreams[i]);
        //join_end[i-1] = getticks();
        //free_start[i-1] = join_end[i-1];
	    //ABT_xstream_free(&xstreams[i]);
        //free_end[i-1] = getticks();
    }

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
  //end_ticks = getticks();

  ABT_finalize();
  printf("Done! Writing to file...");
  //gettimeofday(&stop, NULL);
  //float elapsed_time = (float)(stop.tv_sec - start.tv_sec +
  //              (stop.tv_usec - start.tv_usec)/(float)1000000);
   
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
        //for(int count = 0; count < NUM_REPEAT; count++) {
        printf( "#ESs : %d\n #ULTs : %d\n Real us : %lld \nReal cycles : %lld \nPAPI_TOT_CYC : %lld \nPAPI_TOT_INS : %lld \n PAPI_REF_CYC : %lld\n IPC : %.2lf\n",
                num_threads, loop_count, elapsed_us, elapsed_cyc, values[0], values[1], values[2],  ipc);
        fprintf(afp, "%d %d %lld %lld %lld %lld %lld %.2lf\n",
                num_threads, loop_count, elapsed_us, elapsed_cyc, values[0], values[1], values[2], ipc);
        //for (int i=0; i < NUM_ITERATIONS; i++) {
        //    fprintf(afp, "%d %llu\n", loop_count, diff_ticks[i]);
        //}
        /*printf("%d %f %f %f\n", num_threads, (create)/(num_threads-1),
                            (join)/(num_threads-1),
                            (free)/(num_threads-1));
        fprintf(afp, "%d %f %f %f\n", num_threads, (create)/(num_threads-1),
                            (join)/(num_threads-1),
                            (free)/(num_threads-1));*/
        fclose(afp);
  }
    PAPI_shutdown();
    printf("Done!\n");
}

