#include <stdbool.h>


#include "lock.h"
#include "../common/alloc.h"

struct lock_struct {
    bool flags[MAX_THREADS];
    int tail;
    int nthreads;
};

__thread int mySlot;

lock_t *lock_init(int nthreads)
{
    lock_t *lock;
    XMALLOC(lock, 1);
    lock->nthreads = nthreads;
    lock->tail = 0;
    lock->flags[0] = true;
    return lock;
}

void lock_free(lock_t *lock)
{
    XFREE(lock);
}

void lock_acquire(lock_t *lock)
{
    mySlot = __atomic_fetch_add(&lock->tail, 1, __ATOMIC_RELAXED) % lock->nthreads;
    while(lock->flags[mySlot] != true);
    lock->flags[mySlot] = false;
}

void lock_release(lock_t *lock)
{
    __atomic_store_n(&lock->flags[(mySlot + 1) % lock->nthreads], true, __ATOMIC_RELEASE);
}
