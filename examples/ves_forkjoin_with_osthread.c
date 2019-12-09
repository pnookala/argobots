#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <abt.h>
#include <unistd.h>
#include <sys/time.h>
#include <sched.h>
#define NUM_ES  1
#define NUM_ITERATIONS 2 //10000

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
  int num_threads, loop_count, ret;
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
  ticks os_ticks;
  ticks diff_create_ticks;
  ticks diff_join_ticks;
  //float *times;

  //times = (float*)calloc(loop_count, sizeof(float));
   /* initialization */
  //  gettimeofday(&start, NULL);
  //cpu_set_t my_cpu;
    /* Skip CPU0 - let the OS run on that one */
    //int my_cpu_num = 10;

    //CPU_ZERO (&my_cpu);
    //CPU_SET (my_cpu_num, &my_cpu);
    //if (sched_setaffinity (0, sizeof(my_cpu), &my_cpu) == -1)
    //    printf ("setaffinity failed\n");
    
  ABT_init(0, NULL);
  pools = (ABT_pool *)malloc(sizeof(ABT_pool) * num_threads);
  /* ES creation */
  xstreams = (ABT_xstream *)malloc(sizeof(ABT_xstream) * num_threads);
    
  //start_ticks = (ticks*)calloc(loop_count, sizeof(ticks));
  //end_ticks = (ticks*)calloc(loop_count, sizeof(ticks));
  //diff_create_ticks = (ticks*)calloc(loop_count, sizeof(ticks));
  //diff_join_ticks = (ticks*)calloc(loop_count, sizeof(ticks));
  //ABT_xstream_self(&xstreams[0]);
  
  /* warm up */
  //gettimeofday(&start, NULL);
  start_ticks = getticks();
  for (i = 0; i < num_threads; i++) {
    ABT_xstream_create(ABT_SCHED_NULL, &xstreams[i]);
  }
    end_ticks = getticks();
    diff_create_ticks = (end_ticks - start_ticks)/(num_threads);

  start_ticks = getticks();
  for (i=0; i < num_threads; i++) {  
    ABT_xstream_join(xstreams[i]);  
    //ABT_xstream_free(&xstreams[i]);
  }
  end_ticks = getticks();

  diff_join_ticks = (end_ticks - start_ticks)/(num_threads);
  //gettimeofday(&stop, NULL);

  //float osthread_time = (float)(stop.tv_sec - start.tv_sec +
  //              (stop.tv_usec - start.tv_usec)/(float)1000000);
  //osthread_time = osthread_time/(float)(num_threads-1);
  
  //printf("Created master threads in %llu ticks...\n", os_ticks);
  /* actual run */
  for (k = 0; k < loop_count; k++) {
    //printf("iteration %d\n", k);
    start_ticks = getticks();
    //gettimeofday(&start, NULL);
    for (i = 0; i < 72; i++) {
        //start_ticks[i-1] = getticks();
        es_create(&xstreams[i]);
        //ABT_xstream_create(ABT_SCHED_NULL, &xstreams[i]);
    }
    end_ticks = getticks();
    diff_create_ticks = (end_ticks - start_ticks)/72;
    start_ticks = getticks();
    for (i = 0; i < 72; i++) { 
        //ABT_xstream_get_main_pools(xstreams[i], 1, &pools[i]);
        //start_ticks[i-1] = getticks();
        //ABT_thread_create(pools[i], noop, NULL,
        //                      ABT_THREAD_ATTR_NULL, NULL);
        //diff_ticks[i-1] = (getticks() - start_ticks[i-1]);
        es_join(&xstreams[i]);
        //ABT_xstream_join(xstreams[i]);
        //ABT_xstream_free(&xstreams[i]);
        //diff_ticks[i-1] = (getticks() - start_ticks[i-1]);
     }
     //gettimeofday(&stop, NULL);
    end_ticks = getticks();
    diff_join_ticks = (end_ticks - start_ticks)/72;
     //float elapsed_time = (float)(stop.tv_sec - start.tv_sec +
     //           (stop.tv_usec - start.tv_usec)/(float)1000000);
     //times[k] = elapsed_time/72.0;
  }

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
        //for (int i=0; i < num_threads-1; i++) {
        fprintf(afp, "%llu %llu\n", diff_create_ticks, diff_join_ticks);
        //}
        /*printf("%d %f %f %f\n", num_threads, (create)/(num_threads-1),
                            (join)/(num_threads-1),
                            (free)/(num_threads-1));
        fprintf(afp, "%d %f %f %f\n", num_threads, (create)/(num_threads-1),
                            (join)/(num_threads-1),
                            (free)/(num_threads-1));*/
        fclose(afp);
  }

    printf("Done!\n");
}

