/* scheduler.cc
 *
 * DESCRIPTION
 * Round robin scheduler for the stackful threads declared in thread.hpp.
 *
 * WHAT IT DOES
 * The threads are kept in a circular linked list built with the "next" field of
 * the TCB (no extra memory, no maximum number of threads). The list always contains
 * main (tcb_main) and every new thread is appended at the end by thread_schedule,
 * which is called by thread_create. thread_yield gives the CPU to the next thread
 * of the list that is READY, so the threads run one after the other in a circle.
 * There is no scheduler object to create: it starts working at the first
 * thread_yield() of main.
 *
 * LIMITS
 * - Cooperative only: a thread keeps the CPU until it calls thread_yield
 * - Terminated threads stay in the list and are skipped at every thread_yield
 * - The next thread is searched linearly, and there are no priorities yet
 */



#include "thread.hpp"





/* TCB of main: it is the first element of the list, and its "next" points to itself,
   so at the beginning the list is made only of main. Its sp is written at the first
   context switch. The fields are: sp, next, args, retval, fn, state */
TCB  tcb_main        = {nullptr, &tcb_main, nullptr, nullptr, nullptr, READY};
TCB  *running_thread = &tcb_main;                                                        
static TCB *tail     = &tcb_main;                                                        





/* Add a new thread at the end of the list.
   The new thread points to main to keep the list circular, then the old last
   thread points to the new one, which becomes the last. */
void thread_schedule(TCB *tcb) {
    tcb->next  = &tcb_main;
    tail->next = tcb;
    tail       = tcb;
}





/* Voluntarily give the CPU to the next READY thread in the list (round robin).
   Terminated threads are skipped. The loop always ends because main is never
   terminated. If the only READY thread is the tcb_main there is nothing to
   switch, so it just returns. */
void thread_yield() {
    TCB *prev = running_thread;
    TCB *next = prev->next;

    while( next->state != READY ) next = next->next;    // skip the terminated threads
    if(next == prev) return;                            // nobody else to run (this works only for tcb_main)

    running_thread = next;
    context_switch(prev, next);                         
}





void thread_start() {
    for(;;) thread_yield();     // idle: give the CPU to the other threads, forever
}