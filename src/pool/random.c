/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 * See COPYRIGHT in top-level directory.
 */

#include "abti.h"

/* Random access pool implementation */

static int      random_init(ABT_pool pool, ABT_pool_config config);
static int      random_free(ABT_pool pool);
static size_t   random_get_size(ABT_pool pool);
static void     random_push_shared(ABT_pool pool, ABT_unit unit);
static void     random_push_private(ABT_pool pool, ABT_unit unit);
static ABT_unit random_pop_shared(ABT_pool pool);
static ABT_unit random_pop_private(ABT_pool pool);
static ABT_unit random_pop_timedwait(ABT_pool pool, double abstime_secs);
static int      random_remove_shared(ABT_pool pool, ABT_unit unit);
static int      random_remove_private(ABT_pool pool, ABT_unit unit);
static int      random_print_all(ABT_pool pool, void *arg,
                               void (*print_fn)(void *, ABT_unit));

typedef ABTI_unit unit_t;
static ABT_unit_type unit_get_type(ABT_unit unit);
static ABT_thread unit_get_thread(ABT_unit unit);
static ABT_task unit_get_task(ABT_unit unit);
static ABT_bool unit_is_in_pool(ABT_unit unit);
static ABT_unit unit_create_from_thread(ABT_thread thread);
static ABT_unit unit_create_from_task(ABT_task task);
static void unit_free(ABT_unit *unit);

struct data {
    ABTI_spinlock mutex;
    size_t num_units;
    unit_t **p_units;
    int cur_index;
    int last_index;
};
typedef struct data data_t;

static inline data_t *pool_get_data_ptr(void *p_data)
{
    return (data_t *)p_data;
}


/* Obtain the FIFO pool definition according to the access type */
int ABTI_pool_get_random_def(ABT_pool_access access, ABT_pool_def *p_def)
{
    int abt_errno = ABT_SUCCESS;

    /* Definitions according to the access type */
    /* FIXME: need better implementation, e.g., lock-free one */
    switch (access) {
        case ABT_POOL_ACCESS_PRIV:
            p_def->p_push   = random_push_private;
            p_def->p_pop    = random_pop_private;
            p_def->p_remove = random_remove_private;
            break;

        case ABT_POOL_ACCESS_SPSC:
        case ABT_POOL_ACCESS_MPSC:
        case ABT_POOL_ACCESS_SPMC:
        case ABT_POOL_ACCESS_MPMC:
            p_def->p_push   = random_push_shared;
            p_def->p_pop    = random_pop_shared;
            p_def->p_remove = random_remove_shared;
            break;

        default:
            ABTI_CHECK_TRUE(0, ABT_ERR_INV_POOL_ACCESS);
    }

    /* Common definitions regardless of the access type */
    p_def->access               = access;
    p_def->p_init               = random_init;
    p_def->p_free               = random_free;
    p_def->p_get_size           = random_get_size;
    p_def->p_pop_timedwait      = random_pop_timedwait;
    p_def->p_print_all          = random_print_all;
    p_def->u_get_type           = unit_get_type;
    p_def->u_get_thread         = unit_get_thread;
    p_def->u_get_task           = unit_get_task;
    p_def->u_is_in_pool         = unit_is_in_pool;
    p_def->u_create_from_thread = unit_create_from_thread;
    p_def->u_create_from_task   = unit_create_from_task;
    p_def->u_free               = unit_free;

  fn_exit:
    return abt_errno;

  fn_fail:
    HANDLE_ERROR_FUNC_WITH_CODE(abt_errno);
    goto fn_exit;
}


/* Pool functions */

int random_init(ABT_pool pool, ABT_pool_config config)
{
    ABTI_UNUSED(config);
    int abt_errno = ABT_SUCCESS;
    ABT_pool_access access;

    data_t *p_data = (data_t *)ABTU_malloc(sizeof(data_t));

    ABT_pool_get_access(pool, &access);

    if (access != ABT_POOL_ACCESS_PRIV) {
        /* Initialize the mutex */
        ABTI_spinlock_create(&p_data->mutex);
    }

    p_data->num_units = 0;
    p_data->cur_index = 0;
    p_data->last_index = 0;

    p_data->p_units = (unit_t**) ABTU_malloc(sizeof(unit_t*) * gp_ABTI_global->max_vxstreams);
    ABT_pool_set_data(pool, p_data);

    return abt_errno;
}

static int random_free(ABT_pool pool)
{
    int abt_errno = ABT_SUCCESS;
    ABT_pool_access access;
    ABTI_pool *p_pool = ABTI_pool_get_ptr(pool);
    void *data = ABTI_pool_get_data(p_pool);
    data_t *p_data = pool_get_data_ptr(data);

    ABT_pool_get_access(pool, &access);
    if (access != ABT_POOL_ACCESS_PRIV) {
        ABTI_spinlock_free(&p_data->mutex);
    }

    ABTU_free(p_data);

    return abt_errno;
}

static size_t random_get_size(ABT_pool pool)
{
    ABTI_pool *p_pool = ABTI_pool_get_ptr(pool);
    void *data = ABTI_pool_get_data(p_pool);
    data_t *p_data = pool_get_data_ptr(data);
    return p_data->num_units - p_data->cur_index;
}

static void random_push_shared(ABT_pool pool, ABT_unit unit)
{
    ABTI_pool *p_pool = ABTI_pool_get_ptr(pool);
    void *data = ABTI_pool_get_data(p_pool);
    data_t *p_data = pool_get_data_ptr(data);
    unit_t *p_unit = (unit_t *)unit;

    ABTI_spinlock_acquire(&p_data->mutex);
    p_data->p_units[p_data->num_units++] = p_unit;

    p_unit->pool = pool;
    ABTI_spinlock_release(&p_data->mutex);
}

static void random_push_private(ABT_pool pool, ABT_unit unit)
{
    ABTI_pool *p_pool = ABTI_pool_get_ptr(pool);
    void *data = ABTI_pool_get_data(p_pool);
    data_t *p_data = pool_get_data_ptr(data);
    unit_t *p_unit = (unit_t *)unit;

    p_data->p_units[p_data->num_units++] = p_unit;
    p_unit->pool = pool;
}

static ABT_unit random_pop_timedwait(ABT_pool pool, double abstime_secs)
{
    return NULL;
}

static ABT_unit random_pop_shared(ABT_pool pool)
{
    ABTI_pool *p_pool = ABTI_pool_get_ptr(pool);
    void *data = ABTI_pool_get_data(p_pool);
    data_t *p_data = pool_get_data_ptr(data);
    unit_t *p_unit = NULL;
    ABT_unit h_unit = ABT_UNIT_NULL;

    ABTI_spinlock_acquire(&p_data->mutex);

    if (p_data->num_units > 0 && p_data->cur_index < p_data->num_units) {
    	p_unit = p_data->p_units[p_data->cur_index];
	    p_data->cur_index++;
    	p_unit->pool = ABT_POOL_NULL;

    	h_unit = (ABT_unit)p_unit;
    }
    ABTI_spinlock_release(&p_data->mutex);

    return h_unit;
}

static ABT_unit random_pop_private(ABT_pool pool)
{
    ABTI_pool *p_pool = ABTI_pool_get_ptr(pool);
    void *data = ABTI_pool_get_data(p_pool);
    data_t *p_data = pool_get_data_ptr(data);
    unit_t *p_unit = NULL;
    ABT_unit h_unit = ABT_UNIT_NULL;

    if(p_data->num_units > 0 && p_data->cur_index < p_data->num_units) {
    	p_unit = p_data->p_units[p_data->cur_index];
	p_data->cur_index++;
   	p_unit->pool = ABT_POOL_NULL;
    	h_unit = (ABT_unit)p_unit;
    }
    return h_unit;
}

static int random_remove_shared(ABT_pool pool, ABT_unit unit)
{
    ABTI_pool *p_pool = ABTI_pool_get_ptr(pool);
    void *data = ABTI_pool_get_data(p_pool);
    data_t *p_data = pool_get_data_ptr(data);
    unit_t *p_unit = (unit_t *)unit;

    ABTI_CHECK_TRUE_RET(p_data->num_units != 0, ABT_ERR_POOL);
    ABTI_CHECK_TRUE_RET(p_unit->pool != ABT_POOL_NULL, ABT_ERR_POOL);
    ABTI_CHECK_TRUE_MSG_RET(p_unit->pool == pool, ABT_ERR_POOL, "Not my pool");

    ABTI_spinlock_acquire(&p_data->mutex);
    if (p_data->num_units > 0) {
        p_unit = p_data->p_units[p_data->num_units--];
        p_unit->pool = ABT_POOL_NULL;
    }

    ABTI_spinlock_release(&p_data->mutex);

    return ABT_SUCCESS;
}

static int random_remove_private(ABT_pool pool, ABT_unit unit)
{
    ABTI_pool *p_pool = ABTI_pool_get_ptr(pool);
    void *data = ABTI_pool_get_data(p_pool);
    data_t *p_data = pool_get_data_ptr(data);
    unit_t *p_unit = (unit_t *)unit;

    ABTI_CHECK_TRUE_RET(p_data->num_units != 0, ABT_ERR_POOL);
    ABTI_CHECK_TRUE_RET(p_unit->pool != ABT_POOL_NULL, ABT_ERR_POOL);
    ABTI_CHECK_TRUE_MSG_RET(p_unit->pool == pool, ABT_ERR_POOL, "Not my pool");

    if (p_data->num_units > 0) {
        p_unit = p_data->p_units[p_data->num_units--];
        p_unit->pool = ABT_POOL_NULL;
    }

    return ABT_SUCCESS;
}


static int random_print_all(ABT_pool pool, void *arg,
                          void (*print_fn)(void *, ABT_unit)) {
    ABT_pool_access access;
    ABTI_pool *p_pool = ABTI_pool_get_ptr(pool);
    void *data = ABTI_pool_get_data(p_pool);
    data_t *p_data = pool_get_data_ptr(data);

    ABT_pool_get_access(pool, &access);
    if (access != ABT_POOL_ACCESS_PRIV) {
        ABTI_spinlock_acquire(&p_data->mutex);
    }

    size_t num_units = p_data->num_units;
    unit_t *p_unit = p_data->p_units[num_units];
    while (num_units--) {
        ABTI_ASSERT(p_unit);
        ABT_unit unit = (ABT_unit)p_unit;
        print_fn(arg, unit);
        p_unit = p_data->p_units[num_units-1];
    }

    if (access != ABT_POOL_ACCESS_PRIV) {
        ABTI_spinlock_release(&p_data->mutex);
    }

    return ABT_SUCCESS;
}


/* Unit functions */

static ABT_unit_type unit_get_type(ABT_unit unit)
{
   unit_t *p_unit = (unit_t *)unit;
   return p_unit->type;
}

static ABT_thread unit_get_thread(ABT_unit unit)
{
    ABT_thread h_thread;
    unit_t *p_unit = (unit_t *)unit;
    if (p_unit->type == ABT_UNIT_TYPE_THREAD) {
        h_thread = p_unit->thread;
    } else {
        h_thread = ABT_THREAD_NULL;
    }
    return h_thread;
}

static ABT_task unit_get_task(ABT_unit unit)
{
    ABT_task h_task;
    unit_t *p_unit = (unit_t *)unit;
    if (p_unit->type == ABT_UNIT_TYPE_TASK) {
        h_task = p_unit->task;
    } else {
        h_task = ABT_TASK_NULL;
    }
    return h_task;
}

static ABT_bool unit_is_in_pool(ABT_unit unit)
{
    unit_t *p_unit = (unit_t *)unit;
    return (p_unit->pool != ABT_POOL_NULL) ? ABT_TRUE : ABT_FALSE;
}

static ABT_unit unit_create_from_thread(ABT_thread thread)
{
    ABTI_thread *p_thread = ABTI_thread_get_ptr(thread);
    unit_t *p_unit = &p_thread->unit_def;
    p_unit->p_prev = NULL;
    p_unit->p_next = NULL;
    p_unit->pool   = ABT_POOL_NULL;
    p_unit->thread = thread;
    p_unit->type   = ABT_UNIT_TYPE_THREAD;

    return (ABT_unit)p_unit;
}

static ABT_unit unit_create_from_task(ABT_task task)
{
    ABTI_task *p_task = ABTI_task_get_ptr(task);
    unit_t *p_unit = &p_task->unit_def;
    p_unit->p_prev = NULL;
    p_unit->p_next = NULL;
    p_unit->pool   = ABT_POOL_NULL;
    p_unit->task   = task;
    p_unit->type   = ABT_UNIT_TYPE_TASK;

    return (ABT_unit)p_unit;
}

static void unit_free(ABT_unit *unit)
{
    *unit = ABT_UNIT_NULL;
}

