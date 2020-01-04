#include <pthread.h>

#include "lock.h"
#include "../common/alloc.h"

struct lock_struct {
    pthread_spinlock_t spinlock;
};

lock_t *lock_init(int nthreads)
{
    lock_t *lock;

    XMALLOC(lock, 1);
    pthread_spin_init(&lock->spinlock, PTHREAD_PROCESS_SHARED);
    return lock;
}

void lock_free(lock_t *lock)
{
    XFREE(lock);
}

void lock_acquire(lock_t *lock)
{
    while(pthread_spin_trylock(&lock->spinlock) != 0);
}

void lock_release(lock_t *lock)
{
    pthread_spin_unlock(&lock->spinlock);
}
