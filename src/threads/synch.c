/*! \file synch.c
 *
 * Implementation of various thread synchronization primitives.
 */

/* This file is derived from source code for the Nachos
   instructional operating system.  The Nachos copyright notice
   is reproduced in full below. */

/* Copyright (c) 1992-1996 The Regents of the University of California.
   All rights reserved.

   Permission to use, copy, modify, and distribute this software
   and its documentation for any purpose, without fee, and
   without written agreement is hereby granted, provided that the
   above copyright notice and the following two paragraphs appear
   in all copies of this software.

   IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE TO
   ANY PARTY FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR
   CONSEQUENTIAL DAMAGES ARISING OUT OF THE USE OF THIS SOFTWARE
   AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF CALIFORNIA
   HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

   THE UNIVERSITY OF CALIFORNIA SPECIFICALLY DISCLAIMS ANY
   WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
   WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
   PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS ON AN "AS IS"
   BASIS, AND THE UNIVERSITY OF CALIFORNIA HAS NO OBLIGATION TO
   PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR
   MODIFICATIONS.
*/

#include "threads/synch.h"
#include <stdio.h>
#include <string.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

/*! Initializes semaphore SEMA to VALUE.  A semaphore is a
    nonnegative integer along with two atomic operators for
    manipulating it:

    - down or "P": wait for the value to become positive, then
      decrement it.

    - up or "V": increment the value (and wake up one waiting
      thread, if any). */
void sema_init(struct semaphore *sema, unsigned value) {
    ASSERT(sema != NULL);

    sema->value = value;
    list_init(&sema->waiters);
}

/*! Down or "P" operation on a semaphore.  Waits for SEMA's value
    to become positive and then atomically decrements it.

    This function may sleep, so it must not be called within an
    interrupt handler.  This function may be called with
    interrupts disabled, but if it sleeps then the next scheduled
    thread will probably turn interrupts back on. */
void sema_down(struct semaphore *sema) {
    enum intr_level old_level;

    ASSERT(sema != NULL);
    ASSERT(!intr_context());

    old_level = intr_disable();
    while (sema->value == 0) {
        force_blocking_threads_to_recompute_priorities();
        list_push_back(&sema->waiters, &thread_current()->elem);
        thread_block();
    }
    sema->value--;
    intr_set_level(old_level);
}

/*! Down or "P" operation on a semaphore, but only if the
    semaphore is not already 0.  Returns true if the semaphore is
    decremented, false otherwise.

    This function may be called from an interrupt handler. */
bool sema_try_down(struct semaphore *sema) {
    enum intr_level old_level;
    bool success;

    ASSERT(sema != NULL);

    old_level = intr_disable();
    if (sema->value > 0) {
        sema->value--;
        success = true;
    }
    else {
      success = false;
    }
    intr_set_level(old_level);

    return success;
}

/*! Up or "V" operation on a semaphore.  Increments SEMA's value
    and wakes up one thread of those waiting for SEMA, if any.

    This function may be called from an interrupt handler. */
void sema_up(struct semaphore *sema) {
    enum intr_level old_level;

    ASSERT(sema != NULL);

    old_level = intr_disable();
    if (!list_empty(&sema->waiters)) {
        struct list_elem* max_pri_elem = list_max(&(sema->waiters),
                                           &priority_less_func__readyorsemalist,
                                           NULL);
        list_remove(max_pri_elem);  // This is all just effectively list_pop_max
        struct thread* max_pri = list_entry(max_pri_elem, struct thread, elem);
        ASSERT(is_thread(max_pri));
        thread_unblock(max_pri);
    }
    sema->value++;
    if (!intr_context()) thread_priority_conditional_yield();
    intr_set_level(old_level);
}

static void sema_test_helper(void *sema_);

/*! Self-test for semaphores that makes control "ping-pong"
    between a pair of threads.  Insert calls to printf() to see
    what's going on. */
void sema_self_test(void) {
    struct semaphore sema[2];
    int i;

    printf("Testing semaphores...");
    sema_init(&sema[0], 0);
    sema_init(&sema[1], 0);
    thread_create("sema-test", PRI_DEFAULT, sema_test_helper, &sema);
    for (i = 0; i < 10; i++) {
        sema_up(&sema[0]);
        sema_down(&sema[1]);
    }
    printf ("done.\n");
}

/*! Thread function used by sema_self_test(). */
static void sema_test_helper(void *sema_) {
    struct semaphore *sema = sema_;
    int i;

    for (i = 0; i < 10; i++) {
        sema_down(&sema[0]);
        sema_up(&sema[1]);
    }
}

/*! Initializes LOCK.  A lock can be held by at most a single
    thread at any given time.  Our locks are not "recursive", that
    is, it is an error for the thread currently holding a lock to
    try to acquire that lock.

   A lock is a specialization of a semaphore with an initial
   value of 1.  The difference between a lock and such a
   semaphore is twofold.  First, a semaphore can have a value
   greater than 1, but a lock can only be owned by a single
   thread at a time.  Second, a semaphore does not have an owner,
   meaning that one thread can "down" the semaphore and then
   another one "up" it, but with a lock the same thread must both
   acquire and release it.  When these restrictions prove
   onerous, it's a good sign that a semaphore should be used,
   instead of a lock. */
void lock_init(struct lock *lock) {
    ASSERT(lock != NULL);

    lock->holder = NULL;
    sema_init(&lock->semaphore, 1);
}

/*! Acquires LOCK, sleeping until it becomes available if
    necessary.  The lock must not already be held by the current
    thread.

    This function may sleep, so it must not be called within an
    interrupt handler.  This function may be called with
    interrupts disabled, but interrupts will be turned back on if
    we need to sleep. */
void lock_acquire(struct lock *lock) {
    ASSERT(lock != NULL);
    ASSERT(!intr_context());
    ASSERT(!lock_held_by_current_thread(lock));

    enum intr_level old_level = intr_disable();

    if (lock->holder) { //If this lock is currently held by someone else,
        thread_current()->blocked_by_lock = lock; // We will be blocked by it
                                                  // and donate our pri to them.
        list_push_front(&(lock->holder->donors), &(thread_current()->donor_elem));
    }
    sema_down(&lock->semaphore);
    thread_current()->blocked_by_lock = NULL;
    lock->holder = thread_current();

    intr_set_level(old_level);
}

/*! Tries to acquires LOCK and returns true if successful or false
    on failure.  The lock must not already be held by the current
    thread.

    This function will not sleep, so it may be called within an
    interrupt handler. */
bool lock_try_acquire(struct lock *lock) {
    bool success;

    ASSERT(lock != NULL);
    ASSERT(!lock_held_by_current_thread(lock));

    enum intr_level old_level = intr_disable();

    success = sema_try_down(&lock->semaphore);
    if (success) {
        thread_current()->blocked_by_lock = NULL;
        lock->holder = thread_current();
    }

    intr_set_level(old_level);
    return success;
}

void remove_donors_who_were_waiting_on(struct lock* lock) {
    if (list_empty(&(thread_current()->donors))) {
        return;
    }
    struct list_elem* thread_elem;
    for (thread_elem = list_next(list_head(&(thread_current()->donors)));
         thread_elem != list_tail(&(thread_current()->donors));
         thread_elem = list_next(thread_elem)) {
             struct thread* thread = list_entry(thread_elem, struct thread, donor_elem);
             ASSERT(is_thread(thread));
             if (thread->blocked_by_lock == lock) {
                 list_remove(thread_elem);
             }
    }
}

/*! Releases LOCK, which must be owned by the current thread.

    An interrupt handler cannot acquire a lock, so it does not
    make sense to try to release a lock within an interrupt
    handler. */
void lock_release(struct lock *lock) {
    ASSERT(lock != NULL);
    ASSERT(lock_held_by_current_thread(lock));

    enum intr_level old_level = intr_disable();

    lock->holder = NULL;
    remove_donors_who_were_waiting_on(lock);
    thread_recompute_priority(thread_current());  // b/c we might have lost some donors
    sema_up(&lock->semaphore);
    thread_priority_conditional_yield();  // b/c our priority might have changed

    intr_set_level(old_level);
}

/*! Returns true if the current thread holds LOCK, false
    otherwise.  (Note that testing whether some other thread holds
    a lock would be racy.) */
bool lock_held_by_current_thread(const struct lock *lock) {
    ASSERT(lock != NULL);

    return lock->holder == thread_current();
}

/*! One semaphore in a list. */
struct semaphore_elem {
    struct list_elem elem;              /*!< List element. */
    struct semaphore semaphore;         /*!< This semaphore. */
};

// Undefined behavior if semaphore has no waiters
struct thread* sema_peek_highestpri_waiter(struct semaphore* sema) {
    ASSERT(!(list_empty(&(sema->waiters))));
    struct list_elem* max_pri_elem = list_max(&(sema->waiters),
                                       &priority_less_func__readyorsemalist,
                                       NULL);
    struct thread* max_pri = list_entry(max_pri_elem, struct thread, elem);
    ASSERT(is_thread(max_pri));
    return max_pri;
}

bool semaphore_less_func(const struct list_elem* a, const struct list_elem* b, void *aux UNUSED) {
    struct semaphore* semaphore_a = &(list_entry(a, struct semaphore_elem, elem))->semaphore;
    struct semaphore* semaphore_b = &(list_entry(b, struct semaphore_elem, elem))->semaphore;
    if (list_empty(&(semaphore_a->waiters))) return true;
    if (list_empty(&(semaphore_b->waiters))) return false;
    return sema_peek_highestpri_waiter(semaphore_a)->priority < sema_peek_highestpri_waiter(semaphore_b)->priority;
}

/*! Initializes condition variable COND.  A condition variable
    allows one piece of code to signal a condition and cooperating
    code to receive the signal and act upon it. */
void cond_init(struct condition *cond) {
    ASSERT(cond != NULL);

    list_init(&cond->waiters);
}

/*! Atomically releases LOCK and waits for COND to be signaled by
    some other piece of code.  After COND is signaled, LOCK is
    reacquired before returning.  LOCK must be held before calling
    this function.

    The monitor implemented by this function is "Mesa" style, not
    "Hoare" style, that is, sending and receiving a signal are not
    an atomic operation.  Thus, typically the caller must recheck
    the condition after the wait completes and, if necessary, wait
    again.

    A given condition variable is associated with only a single
    lock, but one lock may be associated with any number of
    condition variables.  That is, there is a one-to-many mapping
    from locks to condition variables.

    This function may sleep, so it must not be called within an
    interrupt handler.  This function may be called with
    interrupts disabled, but interrupts will be turned back on if
    we need to sleep. */
void cond_wait(struct condition *cond, struct lock *lock) {
    struct semaphore_elem waiter;

    ASSERT(cond != NULL);
    ASSERT(lock != NULL);
    ASSERT(!intr_context());
    ASSERT(lock_held_by_current_thread(lock));

    sema_init(&waiter.semaphore, 0);
    list_push_back(&cond->waiters, &waiter.elem);
    lock_release(lock);
    sema_down(&waiter.semaphore);
    lock_acquire(lock);
}

/*! If any threads are waiting on COND (protected by LOCK), then
    this function signals one of them to wake up from its wait.
    LOCK must be held before calling this function.
 
    Returns true if a waiting thread was released. Otherwise,
    returns false since no waiting threads exist.

    An interrupt handler cannot acquire a lock, so it does not
    make sense to try to signal a condition variable within an
    interrupt handler. */
bool cond_signal(struct condition *cond, struct lock *lock UNUSED) {
    ASSERT(cond != NULL);
    ASSERT(lock != NULL);
    ASSERT(!intr_context ());
    ASSERT(lock_held_by_current_thread (lock));

    if (!list_empty(&cond->waiters)) {
        struct list_elem* max_pri_elem = list_max(&cond->waiters, &semaphore_less_func, NULL);
        list_remove(max_pri_elem);  // This is all just effectively list_pop_max
        sema_up(&list_entry(max_pri_elem, struct semaphore_elem, elem)->semaphore);
        return true;
    } else {
        return false;
    }
}

/*! Wakes up all threads, if any, waiting on COND (protected by
    LOCK).  LOCK must be held before calling this function.
 
    Returns true if at least one waiting thread was released.
    Otherwise, returns false since no waiting threads exist.

    An interrupt handler cannot acquire a lock, so it does not
    make sense to try to signal a condition variable within an
    interrupt handler. */
bool cond_broadcast(struct condition *cond, struct lock *lock) {
    ASSERT(cond != NULL);
    ASSERT(lock != NULL);

    bool signaled = false;
    while (cond_signal(cond, lock)) {
        signaled = true;
    }
    return signaled;
}

/*! Initializes LOCK.*/
void rw_init(struct read_write_lock *lock) {
    ASSERT(lock != NULL);
    
    lock_init(&lock->user);
    cond_init(&lock->waiting_readers);
    cond_init(&lock->waiting_writers);
    lock->is_acquired_by_writer = false;
    lock->reader_count = false;
}

/*! Acquires LOCK for reading, sleeping until it becomes available if
    necessary.  Note that a lock is availible for reading if there are
    no writers using it.  The lock must not already be held by the
    current thread.

    This function may sleep, so it must not be called within an
    interrupt handler.  This function may be called with
    interrupts disabled, but interrupts will be turned back on if
    we need to sleep. */
void rw_read_acquire(struct read_write_lock *lock) {
    lock_acquire(&lock->user);
    
    // Wait for the writer to go away...
    while (lock->is_acquired_by_writer) {
        cond_wait(&lock->waiting_readers, &lock->user);
    }
    
    // It's gone now, so we're allowed to read.
    lock->reader_count += 1;
    lock_release(&lock->user);
}

/*! Acquires LOCK for writer, sleeping until it becomes available if
    necessary.  Note that a lock is availible for writing if there are
    no readers and no writers using it.  The lock must not already be
    held by the current thread.

    This function may sleep, so it must not be called within an
    interrupt handler.  This function may be called with
    interrupts disabled, but interrupts will be turned back on if
    we need to sleep. */
void rw_write_acquire(struct read_write_lock *lock) {
    lock_acquire(&lock->user);
    
    // Wait for all writers and readers to go away...
    while (lock->is_acquired_by_writer || lock->reader_count > 0) {
        cond_wait(&lock->waiting_writers, &lock->user);
    }
    
    // They're gone now, so we're allowed to write.
    lock->is_acquired_by_writer = true;
    lock_release(&lock->user);
}


/*! Releases LOCK, which must be owned by the current thread.

    An interrupt handler cannot acquire a lock, so it does not
    make sense to try to release a lock within an interrupt
    handler. */
void rw_read_release(struct read_write_lock *lock) {
    lock_acquire(&lock->user);
    ASSERT(!lock->is_acquired_by_writer);

    // Let in a writer, if one's waiting.
    if (!cond_signal(&lock->waiting_writers, &lock->user)) {
        // Otherwise, let's let all the waiting readers through.
        cond_broadcast(&lock->waiting_readers, &lock->user);
    }
    
    lock_release(&lock->user);
}

/*! Releases LOCK, which must be owned by the current thread.

    An interrupt handler cannot acquire a lock, so it does not
    make sense to try to release a lock within an interrupt
    handler. */
void rw_write_release(struct read_write_lock *lock) {
    lock_acquire(&lock->user);
    ASSERT(lock->is_acquired_by_writer);
    
    // Let in any readers, if some are waiting.
    if (!cond_broadcast(&lock->waiting_readers, &lock->user)) {
        // Otherwise, let's let the next waiting writer through.
        cond_signal(&lock->waiting_writers, &lock->user);
    }
    
    lock_release(&lock->user);
}

