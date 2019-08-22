/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 * See COPYRIGHT in top-level directory.
 */

#ifndef KTHREAD_H_INCLUDED
#define KTHREAD_H_INCLUDED

/* Inlined functions for Kernel threads */

static inline
void ABTI_kthread_set_request(ABTI_kthread *k_thread, uint32_t req)
{
    ABTD_atomic_fetch_or_uint32(&k_thread->request, req);
}

static inline
void ABTI_kthread_unset_request(ABTI_kthreaad *k_thread, uint32_t req)
{
    ABTD_atomic_fetch_and_uint32(&k_thread->request, ~req);
}

/* Add the specified scheduler to the sched stack (field scheds) */
static inline
void ABTI_kthread_push_sched(ABTI_kthread *k_thread, ABTI_ksched *k_sched)
{
    if (k_thread->num_scheds == k_thread->max_scheds) {
        int max_size = k_thread->max_scheds+10;
        void *temp;
        temp = ABTU_realloc(k_threads->v_scheds, max_size*sizeof(ABTI_ksched *));
        k_thread->v_scheds = (ABTI_ksched **)temp;
        k_thread->max_scheds = max_size;
    }

    k_thread->v_scheds[k_thread->num_scheds++] = k_sched;
}

#endif /* KTHREAD_H_INCLUDED */
