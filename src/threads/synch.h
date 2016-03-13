/*! \file synch.h
 *
 * Data structures and function declarations for thread synchronization
 * primitives.
 */

#ifndef THREADS_SYNCH_H
#define THREADS_SYNCH_H

#include <list.h>
#include <stdbool.h>

/*! A counting semaphore. */
struct semaphore {
    unsigned value;             /*!< Current value. */
    struct list waiters;        /*!< List of waiting threads. */
};

void sema_init(struct semaphore *, unsigned value);
void sema_down(struct semaphore *);
bool sema_try_down(struct semaphore *);
void sema_up(struct semaphore *);
void sema_self_test(void);

/*! Lock. */
struct lock {
    struct thread *holder;      /*!< Thread holding lock (for debugging). */
    struct semaphore semaphore; /*!< Binary semaphore controlling access. */
};

void lock_init(struct lock *);
void lock_acquire(struct lock *);
bool lock_try_acquire(struct lock *);
void lock_release(struct lock *);
bool lock_held_by_current_thread(const struct lock *);

/*! Condition variable. */
struct condition {
    struct list waiters;        /*!< List of waiting threads. */
};

void cond_init(struct condition *);
void cond_wait(struct condition *, struct lock *);
bool cond_signal(struct condition *, struct lock *);
bool cond_broadcast(struct condition *, struct lock *);

/*! Read-write lock. */
struct read_write_lock {
    struct lock user;
    struct condition waiting_readers;  /*!< Condition variable of waiting reader threads. */
    struct condition waiting_writers;  /*!< Condition variable of waiting writer threads. */
    bool is_acquired_by_writer;        /*!< True if the lock is acquired by a writer. */
    int reader_count;                  /*!< The number of readers who have acquired a lock. */
};

void rw_init(struct read_write_lock *);
void rw_read_acquire(struct read_write_lock *);
void rw_read_release(struct read_write_lock *);
void rw_write_acquire(struct read_write_lock *);
void rw_write_release(struct read_write_lock *);

/*! Optimization barrier.

   The compiler will not reorder operations across an
   optimization barrier.  See "Optimization Barriers" in the
   reference guide for more information.*/
#define barrier() asm volatile ("" : : : "memory")

#endif /* threads/synch.h */

