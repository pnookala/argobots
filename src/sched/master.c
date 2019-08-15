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

static int  sched_init(ABTI_ksched *sched, ABT_sched_config config);
static void sched_run(ABTI_ksched *sched);
static int  sched_free(ABTI_ksched *sched);

/*static ABT_sched_def sched_master_def = {
    .type = ABT_SCHED_TYPE_ULT,
    .init = sched_init,
    .run = sched_run,
    .free = sched_free,
    .get_migr_pool = NULL,
};*/

typedef struct {
    uint32_t event_freq;
    int num_scheds;
    ABTI_sched **v_scheds;
#ifdef ABT_CONFIG_USE_SCHED_SLEEP
    struct timespec sleep_time;
#endif
} sched_data;

ABT_sched_config_var ABT_sched_master_freq = {
    .idx = 0,
    .type = ABT_SCHED_CONFIG_INT
};

int ABTI_sched_create_master(ABT_sched_config config, ABTI_ksched **newsched) {
   int abt_errno = ABT_SUCCESS;
   ABTI_ksched *p_sched;

   p_sched = (ABTI_ksched *) ABTU_malloc(sizeof(ABTI_ksched));

   //Create the array of execution streams
   p_sched->num_scheds = 0;
   p_sched->v_scheds = (ABTI_sched **) ABTU_malloc(sizeof(ABTI_sched *) * gp_ABTI_global->max_vxstreams);
 
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

static int sched_init(ABTI_ksched *sched, ABT_sched_config config)
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

    p_data->num_scheds = sched->num_scheds;
    p_data->v_scheds = sched->v_scheds;

    sched->data = (void *)p_data;
    return abt_errno;

  /*fn_exit:
    return abt_errno;

  fn_fail:
    HANDLE_ERROR_WITH_CODE("master: sched_init", abt_errno);
    goto fn_exit;*/
}

static void sched_run(ABTI_ksched *p_sched)
{
    uint32_t work_count = 0;
    sched_data *p_data;
    uint32_t event_freq;
    int i;
    int num_scheds;
    CNT_DECL(run_cnt);

    //ABTI_xstream *p_xstream = ABTI_local_get_xstream();

    p_data = sched_data_get_ptr(p_sched->data);
    event_freq = p_data->event_freq;
    num_scheds = p_data->num_scheds;

    while (1) {
        CNT_INIT(run_cnt, 0);

        /* Execute one work unit from the scheduler's pool */
        for (i = 0; i < num_scheds; i++) {
            /* Pop one work unit */
            ABT_unit unit = p_sched->v_scheds[i];
            if (unit != ABT_UNIT_NULL) {
                //ABTI_xstream_run_unit(p_xstream, unit, p_pool);
                CNT_INC(run_cnt);
                break;
            }
        }

        if (++work_count >= event_freq) {
            /*ABTI_xstream_check_events(p_xstream, sched);
            ABT_bool stop = ABTI_sched_has_to_stop(p_sched, p_xstream);
            if (stop == ABT_TRUE)
                break;
            work_count = 0;*/
            SCHED_SLEEP(run_cnt, p_data->sleep_time);
        }
    }
}

static int sched_free(ABTI_ksched *sched)
{
    int abt_errno = ABT_SUCCESS;

    void *data;

    data = sched->data;
    sched_data *p_data = sched_data_get_ptr(data);
    ABTU_free(p_data->v_scheds);
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
