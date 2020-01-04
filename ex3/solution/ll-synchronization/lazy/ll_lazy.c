#include <stdio.h>
#include <stdlib.h> /* rand() */
#include <limits.h>
#include <pthread.h> /* for pthread_spinlock_t */
#include <stdbool.h>

#include "../common/alloc.h"
#include "ll.h"

typedef struct ll_node {
    int key;
    struct ll_node *next;
    pthread_spinlock_t lock;
    bool deleted;
} ll_node_t;

struct linked_list {
    ll_node_t *head;
    /* other fields here? */
};

/**
 * Create a new linked list node.
 **/
static ll_node_t *ll_node_new(int key)
{
    ll_node_t *ret;

    XMALLOC(ret, 1);
    ret->key = key;
    ret->next = NULL;
    ret->deleted = false;
    pthread_spin_init(&ret->lock, PTHREAD_PROCESS_SHARED);

    return ret;
}

/**
 * Free a linked list node.
 **/
static void ll_node_free(ll_node_t *ll_node)
{
    XFREE(ll_node);
}

/**
 * Create a new empty linked list.
 **/
ll_t *ll_new()
{
    ll_t *ret;

    XMALLOC(ret, 1);
    ret->head = ll_node_new(-1);
    ret->head->next = ll_node_new(INT_MAX);
    ret->head->next->next = NULL;

    return ret;
}

/**
 * Free a linked list and all its contained nodes.
 **/
void ll_free(ll_t *ll)
{
    ll_node_t *next, *curr = ll->head;
    while (curr) {
        next = curr->next;
        ll_node_free(curr);
        curr = next;
    }
    XFREE(ll);
}

int validate(ll_node_t *prev, ll_node_t *curr) {
    return (!prev->deleted && !curr->deleted && prev->next == curr);
}

int ll_contains(ll_t *ll, int key)
{
    ll_node_t *curr;

    curr = ll->head;
    while(curr->key < key) {
        curr = curr->next;
    }

    return (curr->key == key && !curr->deleted);
}

int ll_add(ll_t *ll, int key)
{
    ll_node_t *curr, *prev;
    ll_node_t *new_node;
    int ret = 0;
    bool overAndOut = false;

    while(true) {
        prev = ll->head;
        curr = prev->next;

        while(curr->key < key) {
            prev = curr;
            curr = curr->next;
        }

        while(pthread_spin_trylock(&prev->lock) != 0);
        while(pthread_spin_trylock(&curr->lock) != 0);

        if(validate(prev, curr) == 1) {
            if(curr->key != key) {
                ret = 1;
                new_node = ll_node_new(key);
                new_node->next = curr;
                prev->next = new_node;
            }
            overAndOut = true;
        }

        pthread_spin_unlock(&prev->lock);
        pthread_spin_unlock(&curr->lock);

        if(overAndOut)
            break;
    }

    return ret;
}

int ll_remove(ll_t *ll, int key)
{
    ll_node_t *curr, *prev;
    int ret = 0;
    bool overAndOut = false;

    while(true) {
        prev = ll->head;
        curr = prev->next;

        while(curr->key < key) {
            prev = curr;
            curr = curr->next;
        }

        while(pthread_spin_trylock(&prev->lock) != 0);
        while(pthread_spin_trylock(&curr->lock) != 0);

        if(validate(prev, curr) == 1) {
            if(curr->key == key) {
                ret = 1;
                curr->deleted = 1;
                prev->next = curr->next;
            }
            overAndOut = true;
        }

        pthread_spin_unlock(&prev->lock);
        pthread_spin_unlock(&curr->lock);

        if(overAndOut)
            break;
    }

    return ret;
}

/**
 * Print a linked list.
 **/
void ll_print(ll_t *ll)
{
    ll_node_t *curr = ll->head;
    printf("LIST [");
    while (curr) {
        if (curr->key == INT_MAX)
            printf(" -> MAX");
        else
            printf(" -> %d", curr->key);
        curr = curr->next;
    }
    printf(" ]\n");
}

