/* conditionVariable.hpp
 *
 * DESCRIPTION
 * Condition variables to implement control sync
 *
 * WHAT IT DOES
 * A CondVar only holds the list of the threads that wait on it. thread_wait blocks the
 * calling thread (BLOCKED, it leaves the READY list) until another thread calls
 * thread_signal (wakes the thread that waits since more time) or thread_broadcast
 * (wakes all of them). The condition must always be checked in a loop:
 *     while(items == 0) thread_wait(&not_empty);
 * No mutex is needed: the threads are cooperative, so nobody can interrupt a thread
 * between the check of the condition and the thread_wait.
 *
 * LIMITS
 * - A signal or broadcast with nobody waiting is lost: the CondVar has no memory
 * - If main calls thread_wait it does not block, it only yields (main must stay READY)
 * - thread_signal and thread_broadcast must not be called from an interrupt
 * - One CondVar per condition; many threads can wait on or signal the same CondVar
 * - A CondVar must be static or global: the threads wait in its list
 */





#pragma once

#include "scheduler.hpp"





/* A condition to wait for. It contains only the head of the list of the waiting
   threads (nullptr: nobody waits), the TCBs are linked through their "next" field,
   so a CondVar occupies 2 bytes. */
struct CondVar { TCB *wait_list = nullptr; };





/* Block the running thread until somebody calls thread_signal or thread_broadcast on cv */
inline void thread_wait(CondVar *cv) { thread_block(&cv->wait_list);    }



/* Wake up ONE waiting thread: the one that waits since more time (FIFO). Nothing
   happens if nobody is waiting */
inline void thread_signal(CondVar *cv) { thread_wake_one(&cv->wait_list); }



/* Wake up ALL the threads waiting on cv. Nothing happens if nobody is waiting */
inline void thread_broadcast(CondVar *cv) { thread_wake_all(&cv->wait_list); }