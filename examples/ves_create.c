#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <abt.h>
#include <unistd.h>
#include <sys/time.h>
#include <sched.h>
#include <papi.h>
#define NUM_ES  1
#define NUM_ITERATIONS 10000

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

void es_create(ABT_xstream *es) {
    ABT_xstream_create(ABT_SCHED_NULL, es);
}

void es_join(ABT_xstream *es) {
    ABT_xstream_join(*es);
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
  ABT_xstream *vxstreams = NULL;
  ABT_thread *threads = NULL;
  ticks start_ticks;
  ticks end_ticks;
  ticks os_ticks;
  ticks diff_ticks;
   
  long long values[3];
  long long elapsed_us, elapsed_cyc;
  double ipc;
  PAPI_library_init(PAPI_VER_CURRENT);
  int EventSet1 = PAPI_NULL;
  int retval; 
  
  ABT_init(0, NULL);
  pools = (ABT_pool *)malloc(sizeof(ABT_pool) * num_threads);
  /* ES creation */
  xstreams = (ABT_xstream *)malloc(sizeof(ABT_xstream) * num_threads);
  vxstreams = (ABT_xstream *)malloc(sizeof(ABT_xstream) * num_threads);  
  //diff_ticks = (ticks*)calloc(loop_count, sizeof(ticks));
  ABT_xstream_self(&xstreams[0]);
  
  int ret = PAPI_create_eventset(&EventSet1);
    retval=PAPI_add_named_event(EventSet1,"PAPI_TOT_CYC");

    /* Add PAPI_TOT_INS */
    retval=PAPI_add_named_event(EventSet1,"PAPI_TOT_INS");
    /*if (retval!=PAPI_OK) {

        printf( __FILE__, __LINE__, "adding PAPI_TOT_INS", retval );
    }*/
    retval = PAPI_add_named_event(EventSet1, "PAPI_REF_CYC");

  //  elapsed_us = PAPI_get_real_usec(  );
  //  elapsed_cyc = PAPI_get_real_cyc(  );

  //retval = PAPI_start( EventSet1 );
  /* warm up */
  //start_ticks = getticks();
  for (i = 1; i < num_threads; i++) {
    ABT_xstream_create(ABT_SCHED_NULL, &xstreams[i]);
  }

  //retval = PAPI_stop( EventSet1, values );
  //  elapsed_us = (PAPI_get_real_usec(  ) - elapsed_us)/num_threads;
  //  elapsed_cyc = PAPI_get_real_cyc(  ) - elapsed_cyc;
  //end_ticks = getticks();
  //diff_create_ticks = (end_ticks - start_ticks)/(num_threads);
  elapsed_us = PAPI_get_real_usec(  );
  elapsed_cyc = PAPI_get_real_cyc(  );
  retval = PAPI_start( EventSet1 );
  //start_ticks = getticks();
  for (i = 1; i < num_threads; i++) {  
    ABT_xstream_join(xstreams[i]);
    //ABT_xstream_free(&xstreams[i]);
  }
  retval = PAPI_stop( EventSet1, values );
  elapsed_us = (PAPI_get_real_usec(  ) - elapsed_us)/(num_threads-1);
  elapsed_cyc = PAPI_get_real_cyc(  ) - elapsed_cyc;
  //end_ticks = getticks();
  //diff_ticks = end_ticks - start_ticks;
  for(i = 1; i < num_threads; i++) {
    ABT_xstream_free(&xstreams[i]);
  }
  //os_ticks = (end_ticks - start_ticks)/71;
  /* warm up for vES */
  /*for(k = 0; k < 50; k++) {
    for (i = 0; i < 72; i++) {
        ABT_xstream_create(ABT_SCHED_NULL, &vxstreams[i]);
        ABT_xstream_join(vxstreams[i]);
    }

    for(i = 0; i < 72; i++) {
        ABT_xstream_free(&vxstreams[i]);
    }
  }*/

  /* actual run */
  for (k = 0; k < loop_count; k++) {
    //This is necessary because this vES gets created on the core with
    //primary ES and lot of switching happens, so cannot use this to 
    //measure the actual time of vES create and join.
    ABT_xstream_create(ABT_SCHED_NULL, &vxstreams[0]);
    ABT_xstream_join(vxstreams[0]);

    //start_ticks = getticks();
    //elapsed_us = PAPI_get_real_usec(  );
    //elapsed_cyc = PAPI_get_real_cyc(  );
    //retval = PAPI_start( EventSet1 );
    for (i = 1; i < 72; i++) {
        ABT_xstream_create(ABT_SCHED_NULL, &vxstreams[i]);
    }
    //retval = PAPI_stop( EventSet1, values );
    //elapsed_us = (PAPI_get_real_usec(  ) - elapsed_us)/(num_threads-1);
    //elapsed_cyc = PAPI_get_real_cyc(  ) - elapsed_cyc;
    //elapsed_us = PAPI_get_real_usec(  );
    //elapsed_cyc = PAPI_get_real_cyc(  );
    //retval = PAPI_start( EventSet1 );
    for (i = 1; i < 72; i++) { 
        ABT_xstream_join(vxstreams[i]);
    }
    //retval = PAPI_stop( EventSet1, values );
    //elapsed_us = (PAPI_get_real_usec(  ) - elapsed_us)/(num_threads-1);
    //elapsed_cyc = PAPI_get_real_cyc(  ) - elapsed_cyc;
    //end_ticks = getticks();
    //diff_ticks[k] = (end_ticks - start_ticks)/71;
    for(i = 0; i < 72; i++) {
        ABT_xstream_free(&vxstreams[i]);
    }
    //diff_ticks = end_ticks - start_ticks;
  }
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
  ABT_finalize();
    //printf("Done! Writing to file...");
  //gettimeofday(&stop, NULL);
  //float elapsed_time = (float)(stop.tv_sec - start.tv_sec +
  //              (stop.tv_usec - start.tv_usec)/(float)1000000);
  
  if(summary_file != NULL) {
        FILE *afp = fopen(summary_file, "a");
        //fprintf(afp, "%llu\n", diff_ticks/(num_threads-1));
        printf( "#ESs : %d\n #ULTs : %d\n Real us : %lld \nReal cycles : %lld \nPAPI_TOT_CYC : %lld \nPAPI_TOT_INS : %                lld \n PAPI_REF_CYC : %lld\n IPC : %.2lf\n",
                num_threads, loop_count, elapsed_us, elapsed_cyc, values[0], values[1], values[2],  ipc);
        fprintf(afp, "%d %d %lld %lld %lld %lld %lld %.2lf\n",
                num_threads, loop_count, elapsed_us, elapsed_cyc, values[0], values[1], values[2], ipc);
        //for (int i=0; i < loop_count; i++) {
        //    fprintf(afp, "%llu\n", diff_ticks[i]);//diff_create_ticks[i], diff_join_ticks[i],
                                //(diff_create_ticks[i]+diff_join_ticks[i]));
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

