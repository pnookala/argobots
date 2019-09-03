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
void ABTI_kthread_unset_request(ABTI_kthread *k_thread, uint32_t req)
{
    ABTD_atomic_fetch_and_uint32(&k_thread->request, ~req);
}

#endif /* KTHREAD_H_INCLUDED */
