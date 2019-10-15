/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 * See COPYRIGHT in top-level directory.
 */

#include "abti.h"

/** @defgroup SCHED_MASTER Master scheduler
 * This group is for the master scheudler that schedules schedulers.
 */

#ifdef ABT_CONFIG_USE_DEBUG_LOG
static inline uint64_t ABTI_sched_get_new_id(void);
#endif

static int  sched_init(ABT_sched sched, ABT_sched_config config);
static void sched_run(ABT_sched sched);
static int  sched_free(ABT_sched sched);

/*static ABT_sched_def sched_master_def = {
    .type = ABT_SCHED_TYPE_ULT,
    .init = sched_init,
    .run = sched_run,
    .free = sched_free,
    .get_migr_pool = NULL,
};*/

typedef struct {
    uint32_t event_freq;
    int num_pools;
    ABT_pool *pools;
#ifdef ABT_CONFIG_USE_SCHED_SLEEP
    struct timespec sleep_time;
#endif
} sched_data;

ABT_sched_config_var ABT_sched_master_freq = {
    .idx = 0,
    .type = ABT_SCHED_CONFIG_INT
};

int ABTI_sched_create_master(ABT_sched_config config, ABTI_sched **newsched) {
   int abt_errno = ABT_SUCCESS;
   ABTI_sched *p_sched;
   int p;

   p_sched = (ABTI_sched *) ABTU_malloc(sizeof(ABTI_sched));

   //Create the array of execution streams
   p_sched->num_pools = 1;
   p_sched->pools = (ABT_pool *) ABTU_malloc(p_sched->num_pools * sizeof(ABT_pool));
   /* Create random access pool here */
   for (p = 0; p < p_sched->num_pools; p++) {
	abt_errno = ABT_pool_create_random(ABT_POOL_ACCESS_MPMC, 
					  ABT_TRUE, &p_sched->pools[p]);
   }  
 
   p_sched->used	= ABTI_SCHED_NOT_USED;
   p_sched->automatic	= ABT_TRUE;
   p_sched->state	= ABT_SCHED_STATE_READY;
   p_sched->request	= 0;
   p_sched->p_thread 	= NULL;
   p_sched->p_ctx	= NULL;

   p_sched->init	= sched_init;
   p_sched->run		= sched_run;
   p_sched->free	= sched_free;

#ifdef ABT_CONFIG_USE_DEBUG_LOG
   p_sched->id		= ABTI_sched_get_new_id();
#endif

   LOG_EVENT("[Master S%" PRIu64 "] created\n", p_sched->id);

   /* Return value */
   *newsched = p_sched;

   /* Initialize the scheduler */
   p_sched->init(*newsched, config);

   return abt_errno;
}

static inline sched_data *sched_data_get_ptr(void *data)
{
    return (sched_data *)data;
}

static int sched_init(ABT_sched sched, ABT_sched_config config)
{
    int abt_errno = ABT_SUCCESS;

    /* Default settings */
    sched_data *p_data = (sched_data *)ABTU_malloc(sizeof(sched_data));
    p_data->event_freq = ABTI_global_get_sched_event_freq();
#ifdef ABT_CONFIG_USE_SCHED_SLEEP
    p_data->sleep_time.tv_sec = 0;
    p_data->sleep_time.tv_nsec = ABTI_global_get_sched_sleep_nsec();
#endif

    /* Set the variables from the config */
    ABT_sched_config_read(config, 1, &p_data->event_freq);

    int num_pools;
    //p_data->num_pools = sched->num_pools;
    ABT_sched_get_num_pools(sched, &num_pools);
    //p_data->pools = sched->pools;
    p_data->num_pools = num_pools;
    p_data->pools = (ABT_pool *)ABTU_malloc(num_pools * sizeof(ABT_pool));
    abt_errno = ABT_sched_get_pools(sched, num_pools, 0, p_data->pools);
    ABTI_CHECK_ERROR(abt_errno);

    //sched->data = (void *)p_data;
    abt_errno = ABT_sched_set_data(sched, (void *)p_data);

  fn_exit:
    return abt_errno;

  fn_fail:
    HANDLE_ERROR_WITH_CODE("master: sched_init", abt_errno);
    goto fn_exit;
}

static void sched_run(ABT_sched sched)
{
    uint32_t work_count = 0;
    sched_data *p_data;
    uint32_t event_freq;
    int i;
    CNT_DECL(run_cnt);
    ABTI_sched *p_sched = ABTI_sched_get_ptr(sched);
    ABTI_kthread *k_thread = ABTI_local_get_kthread();

    p_data = sched_data_get_ptr(p_sched->data);
    event_freq = p_data->event_freq;
    ABT_pool *pools;
    pools = p_data->pools;

    while (1) {
        CNT_INIT(run_cnt, 0);

        /* Execute one work unit from the scheduler's pool */
        for (i = 0; i < p_data->num_pools; i++) {
            /* Pop one work unit */
	    ABT_pool pool = pools[i];
            ABTI_pool *p_pool = ABTI_pool_get_ptr(pool);
            /* Pop one work unit */
            ABT_unit unit = ABTI_pool_pop(p_pool);
            if (unit != ABT_UNIT_NULL) {
                ABTI_kthread_run_unit(k_thread, unit, p_pool);
		        CNT_INC(run_cnt);
                break;
            }
        }

        if (++work_count >= event_freq) {
	    ABTI_kthread_check_events(k_thread, sched);
	    ABT_bool stop = ABTI_master_sched_has_to_stop(k_thread->k_main_sched, k_thread);
	    if (stop == ABT_TRUE)
                break;
            work_count = 0;
            SCHED_SLEEP(run_cnt, p_data->sleep_time);
        }

    }
}

static int sched_free(ABT_sched sched)
{
    int abt_errno = ABT_SUCCESS;

    void *data;
    ABT_sched_get_data(sched, &data);
//    data = sched->data;
    sched_data *p_data = sched_data_get_ptr(data);
    ABTU_free(p_data->pools);
    ABTU_free(p_data);
    return abt_errno;
}

static uint64_t g_sched_id = 0;

#ifdef ABT_CONFIG_USE_DEBUG_LOG
static inline uint64_t ABTI_sched_get_new_id(void)
{
    return (uint64_t)ABTD_atomic_fetch_add_uint64(&g_sched_id, 1);
}
#endif
