/* scheduler.hpp
 *
 * DESCRIPTION
 * Interface of the scheduler (implementation in scheduler.cc).
 *
 * WHAT IT DOES
 * thread_yield gives the CPU to the next READY thread. thread_block, thread_wake_one and
 * thread_wake_all take a thread out of the READY list and put it back: they are the base
 * of the condition variables (the user should use thread_wait, thread_signal and
 * thread_broadcast of conditionVariable.hpp). thread_schedule appends a thread to the
 * READY list and thread_start is the idle loop of main.
 *
 * LIMITS
 * TCB is only declared here: its definition is in thread.hpp.
 */





#pragma once





struct TCB;





void thread_wake_all(TCB **wait_list);   // make READY every thread of the wait list
void thread_wake_one(TCB **wait_list);   // make READY the thread that waits since more time
void thread_block(TCB **wait_list);      // block the running thread on the wait list





void thread_yield();                     // give the CPU to the next READY thread





void thread_schedule(TCB *tcb);          // append a thread at the end of the READY list
void thread_start();                     // idle loop of main, never returns