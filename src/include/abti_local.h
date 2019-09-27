/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 * See COPYRIGHT in top-level directory.
 */

#ifndef LOCAL_H_INCLUDED
#define LOCAL_H_INCLUDED

/* Inlined functions for ES Local Data */

static inline
ABTI_xstream *ABTI_local_get_xstream(void) {
    return lp_ABTI_local->p_xstream;
}

#ifdef ABT_XSTREAM_USE_VIRTUAL
static inline
ABTI_kthread *ABTI_local_get_kthread(void) {
    return lp_ABTI_local->k_thread;
}

static inline
void ABTI_local_set_kthread(ABTI_kthread *k_thread) {
    lp_ABTI_local->k_thread = k_thread;
}

static inline
ABTI_node *ABTI_local_get_current_node(void) {
    return lp_ABTI_local->cur_node;
}

static inline
void ABTI_local_set_current_node(ABTI_node *node) {
    lp_ABTI_local->cur_node = node;
}
#endif

static inline
void ABTI_local_set_xstream(ABTI_xstream *p_xstream) {
    lp_ABTI_local->p_xstream = p_xstream;
}

static inline
ABTI_thread *ABTI_local_get_thread(void) {
    return lp_ABTI_local->p_thread;
}

static inline
void ABTI_local_set_thread(ABTI_thread *p_thread) {
    lp_ABTI_local->p_thread = p_thread;
}

static inline
ABTI_task *ABTI_local_get_task(void) {
    return lp_ABTI_local->p_task;
}

static inline
void ABTI_local_set_task(ABTI_task *p_task) {
    lp_ABTI_local->p_task = p_task;
}

#endif /* LOCAL_H_INCLUDED */

