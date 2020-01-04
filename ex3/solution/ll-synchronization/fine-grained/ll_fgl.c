#include <stdio.h>
#include <stdlib.h> /* rand() */
#include <limits.h>
#include <pthread.h>

#include "../common/alloc.h"
#include "ll.h"

typedef struct ll_node {
    int key;
    pthread_spinlock_t lock;
    struct ll_node *next;
} ll_node_t;

struct linked_list {
    ll_node_t *head;
    pthread_spinlock_t lock;
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
    pthread_spin_init(&ret->lock, PTHREAD_PROCESS_SHARED);
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

int ll_contains(ll_t *ll, int key)
{
    ll_node_t *curr, *prev;
    int ret = 0;
    while(pthread_spin_trylock(&ll->lock) != 0);

    prev = ll->head;
    while(pthread_spin_trylock(&prev->lock) != 0);
    pthread_spin_unlock(&ll->lock);

    curr = prev->next;
    while(pthread_spin_trylock(&curr->lock) != 0);

    while(curr->key < key) {
        pthread_spin_unlock(&prev->lock);
        prev = curr;
        curr = curr->next;
        if(curr)
            while(pthread_spin_trylock(&curr->lock) != 0);
    }

    pthread_spin_unlock(&curr->lock);
    pthread_spin_unlock(&prev->lock);

    ret = (key == curr->key);
    return ret;
}

int ll_add(ll_t *ll, int key)
{
    ll_node_t *curr, *prev;
    ll_node_t *new_node;
    int ret = 0;
    while(pthread_spin_trylock(&ll->lock) != 0);

    prev = ll->head;
    while(pthread_spin_trylock(&prev->lock) != 0);
    pthread_spin_unlock(&ll->lock);

    curr = prev->next;
    while(pthread_spin_trylock(&curr->lock) != 0);

    while(curr->key < key) {
        pthread_spin_unlock(&prev->lock);
        prev = curr;
        curr = curr->next;
        if(curr)
            while(pthread_spin_trylock(&curr->lock) != 0);
    }

    if(key != curr->key) {
        ret = 1;
        new_node = ll_node_new(key);
        new_node->next = curr;
        prev->next = new_node;
    }

    pthread_spin_unlock(&curr->lock);
    pthread_spin_unlock(&prev->lock);

    return ret;
}

int ll_remove(ll_t *ll, int key)
{
    int ret = 0;
    ll_node_t *curr, *prev;
    while(pthread_spin_trylock(&ll->lock) != 0);

    prev = ll->head;
    while(pthread_spin_trylock(&prev->lock) != 0);
    pthread_spin_unlock(&ll->lock);

    curr = prev->next;
    while(pthread_spin_trylock(&curr->lock) != 0);

    while(curr->key < key) {
        pthread_spin_unlock(&prev->lock);
        prev = curr;
        curr = curr->next;
        if(curr)
            while(pthread_spin_trylock(&curr->lock) != 0);
    }

    if(key == curr->key) {
        ret = 1;
        prev->next = curr->next;
        ll_node_free(curr);
    }

    if(ret == 0) {
        pthread_spin_unlock(&curr->lock);
    }
    pthread_spin_unlock(&prev->lock);

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
