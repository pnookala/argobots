/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 * See COPYRIGHT in top-level directory.
 */

#include "abti.h"

/* Random access pool implementation */

static int      tree_init(ABTI_tree *tree);
static int      tree_free(ABTI_tree *tree);
static void     tree_push_shared(ABTI_tree *tree, ABTI_xstream *p_xstream, 
                                        ABTI_node *parent_node);
static ABTI_node* tree_pop_shared(ABTI_tree *tree, 
                                        ABTI_node *parent_node);
typedef ABTI_node node_t;

struct data {
    ABTI_spinlock mutex;
    size_t num_nodes;
    node_t *p_node;
};
typedef struct data data_t;

/* Pool functions */

int ABTI_tree_get_def(ABTI_tree_def *def)
{
    def->t_init = tree_init;
    def->t_push = tree_push_shared;
    def->t_pop = tree_pop_shared;
    def->t_free =  tree_free;
    return ABT_SUCCESS;
}

int tree_init(ABTI_tree *tree)
{
     int abt_errno = ABT_SUCCESS;

    data_t *p_data = (data_t *)ABTU_malloc(sizeof(data_t));

    /* Initialize the mutex */
    ABTI_spinlock_create(&p_data->mutex);

    p_data->num_nodes = 0;

    p_data->p_node = (node_t*) ABTU_malloc(sizeof(node_t));
    tree->data = p_data;
    tree->num_xstreams = 0; 

    return abt_errno;
}

static int tree_free(ABTI_tree *tree)
{
    int abt_errno = ABT_SUCCESS;
    data_t *p_data = tree->data;
    ABTI_spinlock_free(&p_data->mutex);
    ABTU_free(p_data);
    return abt_errno;
}

static void tree_push_shared(ABTI_tree *tree, ABTI_xstream *p_xstream,
                                    ABTI_node* cur_node)
{
    data_t *p_data = tree->data;

    ABTI_spinlock_acquire(&p_data->mutex);
    if(cur_node == NULL)
    {
        ABTI_node *node = (ABTI_node *) ABTU_malloc(sizeof (ABTI_node));
        node->p_xstream = p_xstream;    
        node->parent_node = NULL; 
        p_data->p_node = node;
    } else {
        /* Need to make it a child node */
        ABTI_node *node = (ABTI_node *) ABTU_malloc(sizeof(ABTI_node));
        node->p_xstream = p_xstream;
        node->parent_node = cur_node;

        if (cur_node->child_nodes == NULL)
            cur_node->child_nodes = (ABTI_node **)ABTU_malloc(sizeof(ABTI_node*));
        cur_node->child_nodes[cur_node->child_count++] = node;         
    }

    ABTI_spinlock_release(&p_data->mutex);
}

static ABTI_node* tree_pop_shared(ABTI_tree *tree, ABTI_node *parent_node)
{
    data_t *p_data = tree->data;
    ABTI_node *ret;

    ABTI_spinlock_acquire(&p_data->mutex);
    if(parent_node == NULL) {
       ret = p_data->p_node; 
    }
    else {
        if(parent_node->child_count > 0) {
            ret = parent_node->child_nodes[parent_node->child_count--];   
        }
        else ret = NULL;
    }
    ABTI_spinlock_release(&p_data->mutex);
    return ret;
}

